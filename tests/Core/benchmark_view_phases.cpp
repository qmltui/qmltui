// Phase profile of the model views. Without a sampling profiler on the build
// host, this splits the cost of each view into the phases that matter to a
// terminal application: instantiating delegates, rebuilding the viewport on
// scroll, applying a data change, and an idle layout + paint frame, for
// several delegate shapes, plus resident memory per delegate.
//
//   benchmark_view_phases [--rows N]      run everything, print a table
//   benchmark_view_phases --loop V D SEC  scroll view V with delegate D for
//                                         SEC seconds (for stack sampling)
#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTextStream>
#include <memory>
#include <Qtui/Core/items/ListView.h>
#include <Qtui/Core/items/GridView.h>
#include <Qtui/Core/items/TableView.h>
#include <Qtui/Core/items/TreeView.h>
#include <Qtui/Core/items/Repeater.h>
#include "../TestPaint.h"
#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace {

class CountModel final : public QAbstractListModel {
public:
    explicit CountModel(int count) : m_count(count) {}
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_count; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) return {};
        ++reads;
        return role == Qt::DisplayRole ? QVariant(QStringLiteral("Row %1").arg(index.row() + m_generation)) : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override { return {{Qt::DisplayRole, "display"}}; }
    void touch(int row)
    {
        ++m_generation;
        emit dataChanged(index(row), index(row), {Qt::DisplayRole});
    }
    mutable qint64 reads = 0;
private:
    int m_count;
    int m_generation = 0;
};

const QHash<QString, QString> delegates = {
    {"text", "Text { required property string display; text: display }"},
    {"rect", "Rectangle { id: d; required property string display; height: 1; width: 80; Text { text: d.display } }"},
    {"standard", "StandardItemDelegate {}"},
};

QString viewQml(const QString &view, const QString &delegateName, bool bound)
{
    const QString delegate = delegates.value(delegateName);
    QString body;
    if (view == "ListView")
        body = QStringLiteral("ListView { width: 80; height: 24; model: benchmarkModel; delegate: %1 }");
    else if (view == "GridView")
        body = QStringLiteral("GridView { width: 80; height: 24; cellWidth: 20; cellHeight: 1; model: benchmarkModel; delegate: %1 }");
    else if (view == "TableView")
        body = QStringLiteral("TableView { width: 80; height: 24; model: benchmarkModel; columns: [{title: \"Row\", width: 80}]; delegate: %1 }");
    else
        body = QStringLiteral("TreeView { width: 80; height: 24; model: benchmarkModel; delegate: %1 }");
    return (bound ? QStringLiteral("pragma ComponentBehavior: Bound\n") : QString())
        + QStringLiteral("import Qtui\nimport Qtui.Controls\n") + body.arg(delegate);
}

qint64 residentBytes()
{
    QFile statm(QStringLiteral("/proc/self/statm"));
    if (!statm.open(QIODevice::ReadOnly)) return -1;
    const QList<QByteArray> fields = statm.readAll().split(' ');
    return fields.size() > 1 ? fields[1].toLongLong() * 4096 : -1;
}

struct Phase {
    QString view, delegate;
    bool bound = false;
    int delegatesCreated = 0;
    double createUs = 0;       // total, component create + first frame
    double perDelegateUs = 0;
    double scrollUs = 0;       // per step, layout + paint included
    double changeUs = 0;       // per dataChanged, layout + paint included
    double idleLayoutUs = 0;   // per frame, nothing dirty
    double idlePaintUs = 0;
    qint64 readsPerScrollStep = 0;
};

int g_rows = 100000;

Phase measure(QQmlEngine &engine, const QString &view, const QString &delegateName, bool bound, int steps)
{
    Phase result{view, delegateName, bound};
    CountModel model(g_rows);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(viewQml(view, delegateName, bound).toUtf8(), QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return result;
    }
    qtui::TextGridBuffer buffer;
    buffer.resize(80, 24);

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<qtui::Item *>(object.get());
    if (!root) return result;
    qtui::ItemPaintTraversal::renderSubtree(root, buffer, 0, 0);
    result.createUs = timer.nsecsElapsed() / 1000.0;
    result.delegatesCreated = int(root->childItems().size());
    result.perDelegateUs = result.delegatesCreated ? result.createUs / result.delegatesCreated : 0;

    const auto step = [&](int i) {
        if (auto *list = qobject_cast<qtui::ListView *>(root)) list->setContentY(i);
        else if (auto *grid = qobject_cast<qtui::GridView *>(root)) grid->setContentY(i);
        else if (auto *table = qobject_cast<qtui::TableView *>(root)) table->setSelectedRow(24 + i);
        else if (auto *tree = qobject_cast<qtui::TreeView *>(root)) tree->setCurrentIndex(24 + i);
    };
    model.reads = 0;
    timer.restart();
    for (int i = 1; i <= steps; ++i) {
        step(i);
        qtui::ItemPaintTraversal::renderSubtree(root, buffer, 0, 0);
    }
    result.scrollUs = timer.nsecsElapsed() / 1000.0 / steps;
    result.readsPerScrollStep = model.reads / steps;

    timer.restart();
    for (int i = 0; i < steps; ++i) {
        model.touch(steps + 5 + (i % 10)); // a visible row after the scroll
        qtui::ItemPaintTraversal::renderSubtree(root, buffer, 0, 0);
    }
    result.changeUs = timer.nsecsElapsed() / 1000.0 / steps;

    timer.restart();
    for (int i = 0; i < steps; ++i) qtui::ItemPaintTraversal::layoutSubtree(root);
    result.idleLayoutUs = timer.nsecsElapsed() / 1000.0 / steps;
    timer.restart();
    for (int i = 0; i < steps; ++i) qtui::ItemPaintTraversal::paintSubtree(root, buffer, 0, 0);
    result.idlePaintUs = timer.nsecsElapsed() / 1000.0 / steps;
    return result;
}

// Resident memory per delegate, measured on an eager Repeater.
double bytesPerDelegate(QQmlEngine &engine, const QString &delegateName, int count)
{
    CountModel model(count);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData((QStringLiteral("import Qtui\nimport Qtui.Controls\n"
                                      "Column { width: 80; Repeater { model: benchmarkModel; delegate: %1 } }")
                           .arg(delegates.value(delegateName))).toUtf8(), QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }
    const qint64 before = residentBytes();
    std::unique_ptr<QObject> object(component.create());
    const qint64 after = residentBytes();
    const int created = int(qobject_cast<qtui::Item *>(object.get())->childItems().size());
    return created > 0 ? double(after - before) / created : -1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    qtui::ListView linkList; qtui::GridView linkGrid; qtui::TableView linkTable; qtui::TreeView linkTree; qtui::Repeater linkRepeater;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../qml"));
    const QStringList args = application.arguments();

    if (args.size() >= 5 && args[1] == "--loop") {
        // Scroll forever (for stack sampling with an external debugger).
#ifdef __linux__
        prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0); // let gdb -p attach under Yama
#endif
        CountModel model(100000);
        engine.rootContext()->setContextProperty("benchmarkModel", &model);
        QQmlComponent component(&engine);
        component.setData(viewQml(args[2], args[3], false).toUtf8(), QUrl());
        if (!component.isReady()) { qCritical().noquote() << component.errorString(); return 1; }
        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<qtui::Item *>(object.get());
        qtui::TextGridBuffer buffer;
        buffer.resize(80, 24);
        QElapsedTimer clock;
        clock.start();
        const qint64 seconds = args[4].toLongLong();
        for (int i = 1; clock.elapsed() < seconds * 1000; ++i) {
            if (auto *list = qobject_cast<qtui::ListView *>(root)) list->setContentY(i % 90000);
            else if (auto *grid = qobject_cast<qtui::GridView *>(root)) grid->setContentY(i % 20000);
            else if (auto *table = qobject_cast<qtui::TableView *>(root)) table->setSelectedRow(24 + i % 90000);
            else if (auto *tree = qobject_cast<qtui::TreeView *>(root)) tree->setCurrentIndex(24 + i % 90000);
            qtui::ItemPaintTraversal::renderSubtree(root, buffer, 0, 0);
        }
        return 0;
    }

    const int steps = 1000;
    if (const int at = int(args.indexOf("--rows")); at > 0 && at + 1 < args.size()) g_rows = args[at + 1].toInt();
    QTextStream out(stdout);
    out << "model rows: " << g_rows << "\n";
    out << QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8 %9\n")
               .arg("view", -9).arg("delegate", -9).arg("bound", -5).arg("created", 7).arg("us/deleg", 9)
               .arg("scroll us", 9).arg("change us", 9).arg("idle lay", 8).arg("idle paint");
    // Warm the engine once so the first row is not paying for type compilation.
    measure(engine, "ListView", "text", false, 10);
    for (const QString &view : {"ListView", "GridView", "TableView", "TreeView"}) {
        for (const QString &delegate : {"text", "rect", "standard"}) {
            for (bool bound : {false, true}) {
                if (bound && delegate != "text") continue;
                const Phase p = measure(engine, view, delegate, bound, steps);
                out << QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8 %9\n")
                           .arg(p.view, -9).arg(p.delegate, -9).arg(bound ? "yes" : "no", -5)
                           .arg(p.delegatesCreated, 7).arg(p.perDelegateUs, 9, 'f', 1)
                           .arg(p.scrollUs, 9, 'f', 1).arg(p.changeUs, 9, 'f', 1)
                           .arg(p.idleLayoutUs, 8, 'f', 1).arg(p.idlePaintUs, 10, 'f', 1);
            }
        }
    }
    out << "\nResident bytes per delegate (Repeater, 2000 rows):\n";
    for (const QString &delegate : {"text", "rect", "standard"})
        out << QStringLiteral("  %1 %2\n").arg(delegate, -9).arg(bytesPerDelegate(engine, delegate, 2000), 8, 'f', 0);
    out.flush();
    return 0;
}
