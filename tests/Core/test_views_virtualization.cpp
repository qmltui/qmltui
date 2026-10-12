#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QAbstractListModel>
#include <Qamui/Core/Items/ListView.h>
#include <Qamui/Core/Items/GridView.h>
#include <Qamui/Core/Items/TableView.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <memory>

using namespace qamui;
using qamui::testing::paintItem;

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
    void testTableViewDelegateChooser();
    void testGridViewVirtualization();
};

void TestViewsVirtualization::testListViewPooling() {
    QQmlEngine engine;
    TestCountModel model(1000);
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qamui

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
    paintItem(listView, buffer, 0, 0);

    // Initial visible items (10 height / stride 1 + overscan 2 + 2 = max 14 items)
    const int initialChildren = listView->findChildren<Item*>().size();
    QVERIFY(initialChildren > 0 && initialChildren <= 14);

    // Scroll through 500 rows
    for (int y = 0; y < 500; y += 5) {
        listView->setContentY(y);
        paintItem(listView, buffer, 0, 0);
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
        import Qamui

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
    paintItem(tableView, buffer, 0, 0);

    // For 5000 rows x 5 columns (25,000 total cells), only visible viewport delegates (max 14 rows x 5 cols = 70) are created!
    const int childCount = tableView->findChildren<Item*>().size();
    QVERIFY(childCount > 0 && childCount <= 70);
}

void TestViewsVirtualization::testTableViewDelegateChooser() {
    QQmlEngine engine;
    TestCountModel model(100);
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qamui
        import QtQml.Models
        import Qt.labs.qmlmodels

        TableView {
            width: 80
            height: 10
            model: testModel
            columns: [
                { title: "Col0", width: 20 },
                { title: "Col1", width: 20 }
            ]
            delegate: DelegateChooser {
                DelegateChoice {
                    column: 0
                    delegate: Text {
                        required property string display
                        text: "Col0:" + display
                    }
                }
                DelegateChoice {
                    column: 1
                    delegate: Text {
                        required property string display
                        text: "Col1:" + display
                    }
                }
            }
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *tableView = qobject_cast<TableView*>(object.get());
    QVERIFY(tableView != nullptr);

    TextGridBuffer buffer;
    buffer.resize(80, 10);
    paintItem(tableView, buffer, 0, 0);

    QVERIFY(tableView->childItems().size() > 0);

    // Check that column 0 and column 1 rendered using their respective DelegateChoice delegates
    // Header is row 0, divider is row 1, first data row is row 2
    QString rowText;
    for (int x = 0; x < 40; ++x) {
        rowText.append(QString::fromUtf8(buffer.cellAt(x, 2).character()));
    }
    QVERIFY(rowText.contains("Col0:Item 0"));
    QVERIFY(rowText.contains("Col1:Item 0"));
}

QTEST_MAIN(TestViewsVirtualization)
void TestViewsVirtualization::testGridViewVirtualization() {
    QQmlEngine engine;
    TestCountModel model(100000);
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qamui

        GridView {
            width: 80
            height: 10
            cellWidth: 20
            cellHeight: 2
            model: testModel
            delegate: Text {
                required property string display
                text: display
            }
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *gridView = qobject_cast<GridView*>(object.get());
    QVERIFY(gridView != nullptr);

    TextGridBuffer buffer;
    buffer.resize(80, 10);
    paintItem(gridView, buffer, 0, 0);

    // 4 columns x 5 visible rows, plus one overscan row above and below:
    // at most 7 rows x 4 columns = 28 delegates for 100,000 model rows.
    const int initialChildren = gridView->findChildren<Item*>().size();
    QVERIFY2(initialChildren > 0 && initialChildren <= 28, qPrintable(QString::number(initialChildren)));
    QCOMPARE(gridView->contentHeight(), 100000 / 4 * 2);

    auto textAt = [&](int x, int y) {
        QString line;
        for (int i = 0; i < 20; ++i) line += QString::fromUtf8(buffer.cellAt(x + i, y).character());
        return line.trimmed();
    };
    QCOMPARE(textAt(0, 0), QStringLiteral("Item 0"));
    QCOMPARE(textAt(20, 0), QStringLiteral("Item 1"));
    QCOMPARE(textAt(0, 2), QStringLiteral("Item 4"));

    // Scrolling far down materialises that window and releases the old one.
    gridView->setContentY(1000);
    buffer.clear();
    paintItem(gridView, buffer, 0, 0);
    const int scrolledChildren = gridView->findChildren<Item*>().size();
    QVERIFY2(scrolledChildren <= 28, qPrintable(QString::number(scrolledChildren)));
    QCOMPARE(textAt(0, 0), QStringLiteral("Item 2000"));
    QCOMPARE(textAt(60, 0), QStringLiteral("Item 2003"));

    // A narrower view has fewer columns per row.
    gridView->setContentY(0);
    gridView->setWidth(40);
    buffer.clear();
    paintItem(gridView, buffer, 0, 0);
    QCOMPARE(textAt(0, 2), QStringLiteral("Item 2"));
    QCOMPARE(gridView->contentHeight(), 100000 / 2 * 2);
}

#include "test_views_virtualization.moc"
