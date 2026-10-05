// Per-frame rendering cost for representative scenes.
//
// Each scenario is painted into a TextGridBuffer through the same traversal the
// engine uses, then diffed against the previous frame, so paint and diff cost
// are timed separately. Every scenario runs twice per viewport size: idle (no
// mutation between frames; the diff must be empty) and mutating.
//
// Usage: benchmark_render [--json <path>] [--strict] [--frames <count>]
//   --strict  exit non-zero if an idle frame produced diff bytes

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSize>
#include <QVariantList>
#include <cmath>
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <Canvas.h>
#include <Color.h>
#include <Item.h>
#include <ListView.h>
#include <Sparkline.h>
#include <TableView.h>
#include "../TestPaint.h"

namespace {

class CountModel final : public QAbstractListModel {
public:
    explicit CountModel(int count, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_count(count) {}

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_count;
    }

    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && role == Qt::DisplayRole
            ? QVariant(index.row()) : QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}};
    }

private:
    int m_count;
};

struct Sample {
    qint64 paintNs = 0;
    qint64 diffNs = 0;
    qint64 bytes = 0;
    qint64 updateNs = 0;
};

struct Result {
    QString scenario;
    int width = 0;
    int height = 0;
    bool idle = false;
    qint64 paintMedianUs = 0;
    qint64 paintP95Us = 0;
    qint64 diffMedianUs = 0;
    qint64 diffP95Us = 0;
    qint64 bytesMedian = 0;
    qint64 idleBytesMax = 0;
    qint64 updateMedianUs = 0;
    qint64 updatePaintMedianUs = 0;
};

using Mutation = std::function<void(qtui::Item *, int frame)>;

constexpr int WarmUpFrames = 5;
int measuredFrames = 200;

qint64 percentile(std::vector<qint64> values, double fraction)
{
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    const auto index = static_cast<size_t>(
        std::clamp<double>(std::ceil(values.size() * fraction) - 1, 0, values.size() - 1));
    return values[index];
}

Result summarize(const QString &scenario, int width, int height, bool idle,
                 const std::vector<Sample> &samples)
{
    std::vector<qint64> paint, diff, bytes, update, updatePaint;
    qint64 maxBytes = 0;
    for (const Sample &sample : samples) {
        paint.push_back(sample.paintNs / 1000);
        diff.push_back(sample.diffNs / 1000);
        bytes.push_back(sample.bytes);
        update.push_back(sample.updateNs / 1000);
        updatePaint.push_back((sample.updateNs + sample.paintNs) / 1000);
        maxBytes = std::max(maxBytes, sample.bytes);
    }
    Result result;
    result.scenario = scenario;
    result.width = width;
    result.height = height;
    result.idle = idle;
    result.paintMedianUs = percentile(paint, 0.5);
    result.paintP95Us = percentile(paint, 0.95);
    result.diffMedianUs = percentile(diff, 0.5);
    result.diffP95Us = percentile(diff, 0.95);
    result.bytesMedian = percentile(bytes, 0.5);
    result.idleBytesMax = idle ? maxBytes : 0;
    result.updateMedianUs = percentile(update, 0.5);
    result.updatePaintMedianUs = percentile(updatePaint, 0.5);
    return result;
}

// Measure setters separately as well: cached text parsing can run from setText,
// so paint alone does not describe the cost of changing text.
std::vector<Sample> measureItem(qtui::Item *item, int width, int height,
                                const Mutation &mutate)
{
    qtui::TextGridBuffer previous;
    qtui::TextGridBuffer next;
    previous.resize(width, height);
    next.resize(width, height);

    std::vector<Sample> samples;
    samples.reserve(measuredFrames);
    QElapsedTimer timer;
    for (int frame = 0; frame < WarmUpFrames + measuredFrames; ++frame) {
        timer.start();
        if (mutate)
            mutate(item, frame);
        const qint64 updateNs = mutate ? timer.nsecsElapsed() : 0;
        next.clear();
        timer.start();
        qtui::testing::paintItem(item, next, 0, 0);
        const qint64 paintedNs = timer.nsecsElapsed();
        const std::string ansi = previous.diff(next);
        const qint64 diffedNs = timer.nsecsElapsed();
        if (frame >= WarmUpFrames)
            samples.push_back({paintedNs, diffedNs - paintedNs, static_cast<qint64>(ansi.size()), updateNs});
        previous.swap(next);
    }
    return samples;
}

bool runQmlScenario(QQmlEngine &engine, const QString &name, const QString &qml,
                    int width, int height, const Mutation &mutate,
                    std::vector<Result> &results)
{
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << name << component.errorString();
        return false;
    }
    std::unique_ptr<QObject> object(component.create());
    auto *item = qobject_cast<qtui::Item *>(object.get());
    if (!item) {
        qCritical().noquote() << name << "root is not a qtui::Item";
        return false;
    }
    item->setWidth(width);
    item->setHeight(height);
    if (mutate)
        mutate(item, 0);   // give the idle run a populated scene

    results.push_back(summarize(name, width, height, true,
                                measureItem(item, width, height, nullptr)));
    results.push_back(summarize(name, width, height, false,
                                measureItem(item, width, height, mutate)));
    return true;
}

// Raw buffer cost without any items: clear + full fillRect + diff.
void runBufferFill(int width, int height, std::vector<Result> &results)
{
    for (bool idle : {true, false}) {
        qtui::TextGridBuffer previous;
        qtui::TextGridBuffer next;
        previous.resize(width, height);
        next.resize(width, height);
        std::vector<Sample> samples;
        QElapsedTimer timer;
        for (int frame = 0; frame < WarmUpFrames + measuredFrames; ++frame) {
            const uint32_t background = (!idle && (frame % 2)) ? 0xFF203040u : 0xFF102030u;
            timer.start();
            next.clear();
            next.fillRect(0, 0, width, height, " ", 0xFFFFFFFFu, background);
            const qint64 paintedNs = timer.nsecsElapsed();
            const std::string ansi = previous.diff(next);
            const qint64 diffedNs = timer.nsecsElapsed();
            if (frame >= WarmUpFrames)
                samples.push_back({paintedNs, diffedNs - paintedNs, static_cast<qint64>(ansi.size())});
            previous.swap(next);
        }
        results.push_back(summarize(QStringLiteral("buffer_fill"), width, height, idle, samples));
    }
}

QString textRowsQml()
{
    QString qml = QStringLiteral("import Qtui\nColumn {\n");
    for (int row = 0; row < 50; ++row) {
        qml += QStringLiteral(
            "    Text { objectName: \"row%1\"; text: \"<b>Row %1</b> \\u001b[32mOK\\u001b[0m "
            "lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
            "incididunt ut labore et dolore magna aliqua\" }\n").arg(row);
    }
    qml += QStringLiteral("}\n");
    return qml;
}

QVariantList compositeHistory(int frame)
{
    QVariantList history;
    for (int i = 0; i < 40; ++i)
        history.append(50.0 + 40.0 * std::sin((frame + i) * 0.35));
    return history;
}

QVariantList compositeRows(int frame)
{
    QVariantList rows;
    for (int row = 0; row < 40; ++row) {
        rows.append(QVariantList{1000 + row,
                                 QStringLiteral("process-%1").arg(row),
                                 QStringLiteral("%1%").arg((row * 7 + frame) % 100)});
    }
    return rows;
}

const char *compositeQml = R"(
import Qtui
import Qtui.Extras
import Qtui.Canvas

Item {
    id: root
    property int frame: 0
    property var history: []
    property var rows: []

    Sparkline {
        x: 0; y: 0
        width: Math.floor(root.width / 2)
        height: 6
        values: root.history
        minVal: 0
        maxVal: 100
        color: "green"
    }
    Canvas {
        x: 0; y: 6
        width: Math.floor(root.width / 2)
        height: root.height - 6
        renderMode: Canvas.Braille
        color: "cyan"
        Line { x1: 0; y1: 0; x2: 60; y2: 40 }
        Line { x1: root.frame % 50; y1: 0; x2: 80; y2: 60 }
        Line { x1: 0; y1: 30; x2: 90; y2: (root.frame * 3) % 60 }
        Circle { cx: 40; cy: 30; radius: 12 + (root.frame % 5) }
    }
    TableView {
        x: Math.floor(root.width / 2); y: 0
        width: root.width - Math.floor(root.width / 2)
        height: root.height
        alternatingRows: true
        showGrid: true
        columns: [
            { title: "PID", width: 8 },
            { title: "NAME", width: 18 },
            { title: "CPU", width: 8, align: "right" }
        ]
        model: root.rows
        cellFormat: (row, column, value) => ({
            text: String(value),
            color: column === 2 ? "green" : "white"
        })
    }
}
)";

const char *rectangleQml = R"(
import Qtui
Rectangle {
    color: "#102030"
    border.style: "double"
    border.color: "#56604a"
}
)";

const char *listViewQml = R"(
import Qtui
ListView {
    model: benchmarkModel
    delegate: Rectangle {
        required property int index
        required property string display
        height: 1
        color: ListView.isCurrentItem ? "blue" : "black"
        Text { text: "Row " + display; color: "white" }
    }
}
)";

void printResults(const std::vector<Result> &results)
{
    qInfo("%-14s %9s %-8s %10s %10s %10s %10s %10s %10s",
          "scenario", "size", "mode", "paint_med", "paint_p95", "diff_med", "diff_p95",
          "bytes_med", "idle_max");
    for (const Result &result : results) {
        qInfo("%-14s %4dx%-4d %-8s %8lldus %8lldus %8lldus %8lldus %10lld %10lld",
              qPrintable(result.scenario), result.width, result.height,
              result.idle ? "idle" : "mutating",
              static_cast<long long>(result.paintMedianUs),
              static_cast<long long>(result.paintP95Us),
              static_cast<long long>(result.diffMedianUs),
              static_cast<long long>(result.diffP95Us),
              static_cast<long long>(result.bytesMedian),
              static_cast<long long>(result.idleBytesMax));
    }
}

bool writeJson(const QString &path, const std::vector<Result> &results)
{
    QJsonArray array;
    for (const Result &result : results) {
        array.append(QJsonObject{{QStringLiteral("scenario"), result.scenario},
                                 {QStringLiteral("width"), result.width},
                                 {QStringLiteral("height"), result.height},
                                 {QStringLiteral("mode"), result.idle
                                      ? QStringLiteral("idle") : QStringLiteral("mutating")},
                                 {QStringLiteral("paintMedianUs"), result.paintMedianUs},
                                 {QStringLiteral("paintP95Us"), result.paintP95Us},
                                 {QStringLiteral("diffMedianUs"), result.diffMedianUs},
                                 {QStringLiteral("diffP95Us"), result.diffP95Us},
                                 {QStringLiteral("bytesMedian"), result.bytesMedian},
                                 {QStringLiteral("idleBytesMax"), result.idleBytesMax},
                                 {QStringLiteral("updateMedianUs"), result.updateMedianUs},
                                 {QStringLiteral("updatePaintMedianUs"), result.updatePaintMedianUs},
                                 {QStringLiteral("measuredFrames"), measuredFrames},
                                 {QStringLiteral("warmUpFrames"), WarmUpFrames}});
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical().noquote() << "cannot write" << path;
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    qtui::ListView ensureListViewLinked;
    qtui::TableView ensureTableViewLinked;
    qtui::Sparkline ensureExtrasLinked;
    qtui::Canvas ensureCanvasLinked;

    QString jsonPath;
    bool strict = false;
    const QStringList arguments = application.arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments[i] == QLatin1String("--json") && i + 1 < arguments.size())
            jsonPath = arguments[++i];
        else if (arguments[i] == QLatin1String("--strict"))
            strict = true;
        else if (arguments[i] == QLatin1String("--frames") && i + 1 < arguments.size()) {
            bool valid = false;
            measuredFrames = arguments[++i].toInt(&valid);
            if (!valid || measuredFrames < 1 || measuredFrames > 100000) {
                qCritical("--frames must be between 1 and 100000");
                return 1;
            }
        } else {
            qCritical().noquote() << "Unknown or incomplete argument:" << arguments[i];
            return 1;
        }
    }

    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    CountModel listModel(1000);
    engine.rootContext()->setContextProperty("benchmarkModel", &listModel);

    const Mutation toggleColor = [](qtui::Item *item, int frame) {
        item->setProperty("color", QVariant::fromValue(
            qtui::Color((frame % 2) ? "#302010" : "#102030")));
    };
    const Mutation editOneRow = [](qtui::Item *item, int frame) {
        if (QObject *row = item->findChild<QObject *>(QStringLiteral("row%1").arg(frame % 50)))
            row->setProperty("text", QStringLiteral("<b>Row %1</b> [33mBUSY[0m frame %2 "
                                                    "lorem ipsum dolor sit amet, consectetur")
                                         .arg(frame % 50).arg(frame));
    };
    const Mutation advanceComposite = [](qtui::Item *item, int frame) {
        item->setProperty("frame", frame);
        item->setProperty("history", compositeHistory(frame));
        item->setProperty("rows", compositeRows(frame));
    };
    const Mutation scrollList = [](qtui::Item *item, int frame) {
        item->setProperty("contentY", (frame * 3) % 900);
    };

    std::vector<Result> results;
    bool ok = true;
    for (const QSize size : {QSize(80, 24), QSize(240, 70)}) {
        runBufferFill(size.width(), size.height(), results);
        ok &= runQmlScenario(engine, QStringLiteral("rectangle"), QString::fromUtf8(rectangleQml),
                             size.width(), size.height(), toggleColor, results);
        ok &= runQmlScenario(engine, QStringLiteral("text_rows"), textRowsQml(),
                             size.width(), size.height(), editOneRow, results);
        ok &= runQmlScenario(engine, QStringLiteral("composite"), QString::fromUtf8(compositeQml),
                             size.width(), size.height(), advanceComposite, results);
        ok &= runQmlScenario(engine, QStringLiteral("list_view"), QString::fromUtf8(listViewQml),
                             size.width(), size.height(), scrollList, results);
    }

    printResults(results);
    bool idleClean = true;
    for (const Result &result : results) {
        if (result.idle && result.idleBytesMax > 0) {
            idleClean = false;
            qWarning().noquote() << "idle frame produced output:" << result.scenario
                                 << result.width << "x" << result.height
                                 << result.idleBytesMax << "bytes";
        }
    }
    if (!jsonPath.isEmpty())
        ok &= writeJson(jsonPath, results);

    if (!ok)
        return 1;
    return (strict && !idleClean) ? 2 : 0;
}
