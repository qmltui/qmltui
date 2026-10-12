#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <memory>
#include <QmlTui/Core/Items/TreeView.h>
#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/GridView.h>
#include "../TestPaint.h"

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

    void touch(int row) { emit dataChanged(index(row, 0), index(row, 0), {Qt::DisplayRole}); }

private:
    int m_count;
};

static qint64 measureCreation(QQmlEngine &engine, int rowCount)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        
        TreeView {
            width: 80
            height: 24
            model: benchmarkModel
            delegate: Item {
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool current
                required property string display
                required property var modelData
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto *tree = qobject_cast<qmltui::TreeView*>(object.get());
    if (!tree) return -1;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qmltui::testing::paintItem(tree, buffer, 0, 0);
    if (tree->childItems().size() < 24) {
        qCritical("TreeView benchmark failed to create visible delegates");
        return -1;
    }
    return timer.elapsed();
}

static qint64 measureTableViewDelegate(QQmlEngine &engine, int rowCount)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        
        TableView {
            width: 80
            height: 24
            columns: [{ title: "Row", width: 80 }]
            model: benchmarkModel
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto *table = qobject_cast<qmltui::TableView*>(object.get());
    if (!table) return -1;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qmltui::testing::paintItem(table, buffer, 0, 0);
    if (table->childItems().size() < 22 || buffer.cellAt(0, 2).character() != "0") {
        qCritical("TableView benchmark failed to render visible delegates");
        return -1;
    }
    return timer.elapsed();
}

static qint64 measureListViewSelection(QQmlEngine &engine, int rowCount, int steps)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui

        ListView {
            width: 80
            height: 24
            model: benchmarkModel
            delegate: Rectangle {
                required property int index
                required property string display
                height: 3
                color: ListView.isCurrentItem ? "blue" : "black"
                Text {
                    text: display
                    color: ListView.isCurrentItem ? "white" : "gray"
                }
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }

    std::unique_ptr<QObject> object(component.create());
    auto *listView = qobject_cast<qmltui::ListView*>(object.get());
    if (!listView) return -1;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;

    qmltui::testing::paintItem(listView, buffer, 0, 0);

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < steps; ++i) {
        listView->setCurrentIndex(i % rowCount);
        qmltui::testing::paintItem(listView, buffer, 0, 0);
    }
    return timer.elapsed();
}

// Plan 07 WP6: dataChanged on a visible node must not re-flatten a 10k node
// tree or re-create delegates.
static qint64 measureTreeViewDataChanged(QQmlEngine &engine, int rowCount, int steps)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui

        TreeView {
            width: 80
            height: 24
            model: benchmarkModel
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }
    std::unique_ptr<QObject> object(component.create());
    auto *tree = qobject_cast<qmltui::TreeView*>(object.get());
    if (!tree) return -1;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qmltui::testing::paintItem(tree, buffer, 0, 0);
    const auto before = tree->childItems();

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < steps; ++i) {
        model.touch(i % 24);
        qmltui::testing::paintItem(tree, buffer, 0, 0);
    }
    const qint64 elapsed = timer.elapsed();
    if (tree->childItems() != before) {
        qCritical("TreeView dataChanged re-created delegate items");
        return -1;
    }
    return elapsed;
}

static qint64 measureGridViewScroll(QQmlEngine &engine, int rowCount, int steps)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui

        GridView {
            width: 80
            height: 24
            cellWidth: 20
            cellHeight: 2
            model: benchmarkModel
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto *grid = qobject_cast<qmltui::GridView*>(object.get());
    if (!grid) return -1;
    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qmltui::testing::paintItem(grid, buffer, 0, 0);
    // 4 columns x 12 rows visible, one overscan row each side: at most 56 delegates.
    if (grid->childItems().size() > 56 || buffer.cellAt(0, 0).character() != "0") {
        qCritical("GridView benchmark created more delegates than the viewport needs");
        return -1;
    }
    for (int i = 0; i < steps; ++i) {
        grid->setContentY((i * 7) % grid->contentHeight());
        qmltui::testing::paintItem(grid, buffer, 0, 0);
    }
    return timer.elapsed();
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    qmltui::TreeView ensureModuleIsLinked;
    qmltui::TableView ensureTableViewLinked;
    qmltui::ListView ensureListViewLinked;
    qmltui::GridView ensureGridViewLinked;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../QmlTui"));
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/.."));

    const qint64 tree1kMs = measureCreation(engine, 1'000);
    const qint64 tree10kMs = measureCreation(engine, 10'000);
    qInfo() << "TreeView creation: 1k rows" << tree1kMs
            << "ms; 10k rows" << tree10kMs << "ms";

    const qint64 table1kMs = measureTableViewDelegate(engine, 1'000);
    const qint64 table10kMs = measureTableViewDelegate(engine, 10'000);
    qInfo() << "TableView (Custom Delegate) creation & render: 1k rows" << table1kMs
            << "ms; 10k rows" << table10kMs << "ms";

    const qint64 listSelectMs = measureListViewSelection(engine, 1'000, 1'000);
    qInfo() << "ListView 1,000 selection changes & render steps:" << listSelectMs << "ms"
            << "(" << (listSelectMs / 1000.0) << "ms per selection step)";

    const qint64 treeChangeMs = measureTreeViewDataChanged(engine, 10'000, 1'000);
    qInfo() << "TreeView 10k nodes: 1,000 dataChanged & render steps:" << treeChangeMs << "ms";

    const qint64 gridScrollMs = measureGridViewScroll(engine, 100'000, 1'000);
    qInfo() << "GridView 100k rows: creation + 1,000 scroll & render steps:" << gridScrollMs << "ms";

    return (tree1kMs < 0 || tree10kMs < 0 || table1kMs < 0 || table10kMs < 0 || listSelectMs < 0
            || gridScrollMs < 0 || treeChangeMs < 0) ? 1 : 0;
}
