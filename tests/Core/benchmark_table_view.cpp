#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QDebug>
#include <QmlTui/Core/Items/TableView.h>
#include "../TestPaint.h"
#include <algorithm>
#include <functional>
#include <memory>

class BenchmarkTable final : public QAbstractTableModel {
public:
    explicit BenchmarkTable(int count) : rows(count) {}
    int rows;
    int revision = 0;
    mutable qint64 reads = 0;
    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : rows;
    }
    int columnCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : 4;
    }
    QHash<int, QByteArray> roleNames() const override {
        return {
            {Qt::DisplayRole, "display"}, {Qt::UserRole, "order"}, {Qt::UserRole + 1, "detail"}};
    }
    QVariant data(const QModelIndex& index, int role) const override {
        ++reads;
        if (!index.isValid()) return {};
        if (role == Qt::DisplayRole)
            return QStringLiteral("%1:%2:%3").arg(index.row()).arg(index.column()).arg(revision);
        if (role == Qt::UserRole) return index.row();
        if (role == Qt::UserRole + 1) return QStringLiteral("detail");
        return {};
    }
    void change(int first, int last) {
        ++revision;
        emit dataChanged(index(first, 0), index(last, 3), {Qt::DisplayRole});
    }
};

struct Distribution {
    QList<qint64> samples;
    void print(const char* name) {
        std::sort(samples.begin(), samples.end());
        qInfo().nospace() << name << " median=" << samples[samples.size() / 2] / 1000.0
                          << "us p95=" << samples[(samples.size() * 95) / 100] / 1000.0 << "us";
    }
};

static bool benchmark(QQmlEngine& engine, int rows, bool complex, bool useProxy) {
    BenchmarkTable source(rows);
    QSortFilterProxyModel proxy;
    proxy.setSortRole(Qt::UserRole);
    QElapsedTimer timer;
    timer.start();
    if (useProxy) {
        proxy.setSourceModel(&source);
        proxy.sort(0, Qt::DescendingOrder);
        (void)proxy.rowCount(); // Include lazy mapping construction in setup cost.
    }
    const qint64 sortNs = timer.nsecsElapsed();
    auto* model = useProxy ? static_cast<QAbstractItemModel*>(&proxy) : &source;

    QQmlComponent component(&engine);
    const QByteArray leaf =
        complex ? R"(
        delegate: Rectangle {
            id: cell
            required property string display
            required property string detail
            required property bool current
            color: current ? "blue" : "black"
            Text { text: cell.display; width: 18; color: "white" }
            Rectangle { x: 18; width: 1; height: 1; color: "cyan" }
        }
    )"
                : R"(delegate: Text { required property string display; text: display })";
    component.setData(QByteArray(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        TableView {
            required property var inputModel
            width: 80; height: 24
            columns: [{title:"A",width:20},{title:"B",width:20},{title:"C",width:20},{title:"D",width:20}]
            model: inputModel
    )") + leaf + "}",
                      QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return false;
    }
    std::unique_ptr<QObject> object(
        component.createWithInitialProperties({{"inputModel", QVariant::fromValue(model)}}));
    auto* table = qobject_cast<qmltui::TableView*>(object.get());
    if (!table) return false;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return false;
    auto paint = [&] {
        buffer.clear();
        qmltui::testing::paintItem(table, buffer, 0, 0);
    };
    paint();
    if (table->childItems().isEmpty()) return false;
    qint64 created = 0, destroyed = 0;
    QSet<qmltui::Item*> watched;
    const auto watch = [&] {
        for (auto* item : table->childItems()) {
            if (watched.contains(item)) continue;
            watched.insert(item);
            ++created;
            QObject::connect(item, &QObject::destroyed, table, [&, item] {
                ++destroyed;
                watched.remove(item);
            });
        }
    };
    watch();
    qInfo() << "TableView rows=" << rows << "complex=" << complex << "proxy=" << useProxy
            << "initialSortMs=" << sortNs / 1e6 << "delegates=" << table->childItems().size();

    for (const QString& operation :
         {QStringLiteral("single"), QStringLiteral("broad"), QStringLiteral("scroll")}) {
        Distribution update, render;
        source.reads = 0;
        created = 0;
        destroyed = 0;
        for (int step = 0; step < 120; ++step) {
            timer.start();
            if (operation == "scroll")
                table->setSelectedRow(30 + step);
            else if (operation == "broad")
                source.change(0, rows - 1);
            else
                source.change(useProxy ? rows - 1 : 0, useProxy ? rows - 1 : 0);
            const qint64 updateNs = timer.nsecsElapsed();
            timer.restart();
            paint();
            const qint64 renderNs = timer.nsecsElapsed();
            watch();
            if (step >= 20) {
                update.samples.append(updateNs);
                render.samples.append(renderNs);
            }
        }
        qInfo() << operation << "modelReads=" << source.reads << "created=" << created
                << "destroyed=" << destroyed;
        update.print("signal/proxy");
        render.print("layout/paint");
    }
    // Disconnect counters before their stack storage is destroyed.
    for (auto* item : table->childItems())
        QObject::disconnect(item, nullptr, table, nullptr);
    return true;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qmltui::TableView linked;
    QQmlEngine engine;
    bool ok = true;
    for (int rows : {1'000, 10'000, 1'000'000})
        for (bool complex : {false, true})
            for (bool proxy : {false, true})
                ok &= benchmark(engine, rows, complex, proxy);
    return ok ? 0 : 1;
}
