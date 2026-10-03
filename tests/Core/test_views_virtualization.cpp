#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QAbstractListModel>
#include <ListView.h>
#include <TableView.h>
#include <TextGridBuffer.h>
#include <memory>

using namespace qtui;

class TestCountModel final : public QAbstractListModel {
public:
    explicit TestCountModel(int count, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_count(count) {}

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_count;
    }

    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && role == Qt::DisplayRole
            ? QVariant(QString("Item %1").arg(index.row())) : QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}};
    }

private:
    int m_count;
};

class TestViewsVirtualization : public QObject {
    Q_OBJECT

private slots:
    void testListViewPooling();
    void testTableView2DVirtualization();
};

void TestViewsVirtualization::testListViewPooling() {
    QQmlEngine engine;
    TestCountModel model(1000);
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui

        ListView {
            width: 80
            height: 10
            model: testModel
            reuseItems: true
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());

    QVERIFY(component.isReady());
    std::unique_ptr<QObject> object(component.create());
    auto *listView = qobject_cast<ListView*>(object.get());
    QVERIFY(listView != nullptr);

    TextGridBuffer buffer;
    buffer.resize(80, 10);
    listView->render(buffer, 0, 0);

    // Initial visible items (10 height / stride 1 + overscan 2 + 2 = max 14 items)
    const int initialChildren = listView->findChildren<Item*>().size();
    QVERIFY(initialChildren > 0 && initialChildren <= 14);

    // Scroll through 500 rows
    for (int y = 0; y < 500; y += 5) {
        listView->setContentY(y);
        listView->render(buffer, 0, 0);
    }

    // Children count should stay bounded (pooled & reused)
    const int scrolledChildren = listView->findChildren<Item*>().size();
    QVERIFY(scrolledChildren <= 14);
}

void TestViewsVirtualization::testTableView2DVirtualization() {
    QQmlEngine engine;
    TestCountModel model(5000);
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui

        TableView {
            width: 80
            height: 10
            model: testModel
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());

    QVERIFY(component.isReady());
    std::unique_ptr<QObject> object(component.create());
    auto *tableView = qobject_cast<TableView*>(object.get());
    QVERIFY(tableView != nullptr);

    TextGridBuffer buffer;
    buffer.resize(80, 10);
    tableView->render(buffer, 0, 0);

    // For 5000 rows x 5 columns (25,000 total cells), only visible viewport delegates (max 14 rows x 5 cols = 70) are created!
    const int childCount = tableView->findChildren<Item*>().size();
    QVERIFY(childCount > 0 && childCount <= 70);
}

QTEST_MAIN(TestViewsVirtualization)
#include "test_views_virtualization.moc"
