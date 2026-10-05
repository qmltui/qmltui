#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <memory>
#include <TreeView.h>
#include <TableView.h>
#include <ListView.h>
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

private:
    int m_count;
};

static qint64 measureCreation(QQmlEngine &engine, int rowCount)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        
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
    auto *tree = qobject_cast<qtui::TreeView*>(object.get());
    if (!tree) return -1;
    qtui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qtui::testing::paintItem(tree, buffer, 0, 0);
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
        import Qtui
        
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
    auto *table = qobject_cast<qtui::TableView*>(object.get());
    if (!table) return -1;
    qtui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    qtui::testing::paintItem(table, buffer, 0, 0);
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
        import Qtui

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
    auto *listView = qobject_cast<qtui::ListView*>(object.get());
    if (!listView) return -1;
    qtui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;

    qtui::testing::paintItem(listView, buffer, 0, 0);

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < steps; ++i) {
        listView->setCurrentIndex(i % rowCount);
        qtui::testing::paintItem(listView, buffer, 0, 0);
    }
    return timer.elapsed();
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    qtui::TreeView ensureModuleIsLinked;
    qtui::TableView ensureTableViewLinked;
    qtui::ListView ensureListViewLinked;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../Qtui"));
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/.."));
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../examples/apps/container_fleet"));

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

    return (tree1kMs < 0 || tree10kMs < 0 || table1kMs < 0 || table10kMs < 0 || listSelectMs < 0) ? 1 : 0;
}
