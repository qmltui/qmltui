#include <QtTest/QtTest>
#include <Sparkline.h>
#include <BigText.h>
#include <TableView.h>
#include <TreeView.h>
#include <Repeater.h>
#include <ListView.h>
#include <GridView.h>
#include <ToastManager.h>
#include <TabHeader.h>
#include <Gauge.h>
#include <Paragraph.h>
#include <TextGridBuffer.h>
#include <MouseEvent.h>
#include <KeyEvent.h>
#include <QQmlEngine>
#include <QQmlComponent>

using namespace qtui;

class TestWidgets : public QObject {
    Q_OBJECT

private slots:
    void testSparkline() {
        Sparkline sparkline;
        sparkline.setWidth(10);
        sparkline.setHeight(1);
        sparkline.setValues(QVariantList{10.0, 20.0, 30.0, 40.0, 50.0});

        TextGridBuffer buffer;
        buffer.resize(10, 1);
        sparkline.render(buffer, 0, 0);

        QVERIFY(buffer.cellAt(4, 0).character[0] != ' ');
    }

    void testBigText() {
        BigText bigText;
        bigText.setText("123");

        TextGridBuffer buffer;
        buffer.resize(20, 3);
        bigText.render(buffer, 0, 0);

        QCOMPARE(buffer.cellAt(0, 0).character[0], ' ');
        QVERIFY(buffer.cellAt(1, 0).character[0] != '\0');
    }

    void testTableView() {
        TableView table;
        table.setWidth(40);
        table.setHeight(6);

        table.setColumns(QVariantList{
            QVariantMap{{"title", "ID"}, {"width", 10}},
            QVariantMap{{"title", "NAME"}, {"width", 20}}
        });

        table.setModel(QVariantList{
            QVariantList{"1", "Alice"},
            QVariantList{"2", "Bob"}
        });

        TextGridBuffer buffer;
        buffer.resize(40, 6);
        table.render(buffer, 0, 0);

        QCOMPARE(table.selectedRow(), 0);
        table.setSelectedRow(1);
        QCOMPARE(table.selectedRow(), 1);
    }

    void testTreeView() {
        TreeView tree;
        tree.setWidth(30);
        tree.setHeight(5);

        tree.setRootNode(QVariantMap{
            {"text", "root"},
            {"expanded", true},
            {"children", QVariantList{
                QVariantMap{{"text", "child1"}}
            }}
        });

        TextGridBuffer buffer;
        buffer.resize(30, 5);
        tree.render(buffer, 0, 0);

        QCOMPARE(tree.selectedIndex(), 0);

        // Verify initial state (expanded)
        QVERIFY(tree.isExpanded(0));

        // mousePressEvent toggles expanded state (expanded -> collapsed)
        MouseEvent mouseEv(0, 0, 0);
        tree.mousePressEvent(&mouseEv);
        QVERIFY(mouseEv.isAccepted());
        QCOMPARE(tree.selectedIndex(), 0);
        QVERIFY(!tree.isExpanded(0));

        // Click again toggles collapsed -> expanded
        tree.mousePressEvent(&mouseEv);
        QVERIFY(tree.isExpanded(0));

        // Test collapse on expanded root node
        tree.collapse(0);
        QVERIFY(!tree.isExpanded(0));

        // Test Left key on collapsed node (should remain accepted)
        KeyEvent leftEv(Qt::Key_Left, "");
        tree.keyPressEvent(&leftEv);
        QVERIFY(leftEv.isAccepted());
        QVERIFY(!tree.isExpanded(0));

        // Test Right key to expand
        KeyEvent rightEv(Qt::Key_Right, "");
        tree.keyPressEvent(&rightEv);
        QVERIFY(rightEv.isAccepted());
        QVERIFY(tree.isExpanded(0));

        // Test Left key to collapse
        tree.keyPressEvent(&leftEv);
        QVERIFY(!tree.isExpanded(0));

        // Test tree with duplicate item names
        tree.setRootNode(QVariantMap{
            {"text", "root"},
            {"expanded", true},
            {"children", QVariantList{
                QVariantMap{
                    {"text", "DuplicateName"},
                    {"expanded", true},
                    {"children", QVariantList{ QVariantMap{{"text", "Sub1"}} }}
                },
                QVariantMap{
                    {"text", "DuplicateName"},
                    {"expanded", true},
                    {"children", QVariantList{ QVariantMap{{"text", "Sub2"}} }}
                }
            }}
        });
        // Index 1 is first DuplicateName (expanded)
        // Index 3 is second DuplicateName (expanded)
        QVERIFY(tree.isExpanded(1));
        QVERIFY(tree.isExpanded(3));

        // Collapse first DuplicateName (Index 1)
        tree.collapse(1);
        QVERIFY(!tree.isExpanded(1));
        // Second DuplicateName (now at Index 2) must remain expanded independently!
        QVERIFY(tree.isExpanded(2));
    }

    void testToastManager() {
        ToastManager toast;
        toast.setWidth(30);
        toast.setHeight(5);

        toast.showToast("Test", "Message", "info", 1000);
        TextGridBuffer buffer;
        buffer.resize(30, 5);
        toast.render(buffer, 0, 0);
    }

    void testTabHeader() {
        TabHeader header;
        header.setWidth(40);
        header.setHeight(1);
        header.setTabs(QVariantList{"Overview", "Metrics", "Settings"});

        QCOMPARE(header.currentIndex(), 0);
        header.setCurrentIndex(1);
        QCOMPARE(header.currentIndex(), 1);

        TextGridBuffer buffer;
        buffer.resize(40, 1);
        header.render(buffer, 0, 0);
    }

    void testGauge() {
        Gauge gauge;
        gauge.setWidth(30);
        gauge.setHeight(1);
        gauge.setLabel("CPU: ");
        gauge.setValue(75.0);

        TextGridBuffer buffer;
        buffer.resize(30, 1);
        gauge.render(buffer, 0, 0);

        QCOMPARE(gauge.value(), 75.0);
    }

    void testParagraph() {
        Paragraph p;
        p.setWidth(20);
        p.setHeight(5);
        p.setText("Qtui is a modern reactive QML Terminal UI framework.");

        TextGridBuffer buffer;
        buffer.resize(20, 5);
        p.render(buffer, 0, 0);
    }

    void testRequiredProperties() {
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));

        QQmlComponent component(&engine);
        component.setData(QByteArray(
            "import QtQuick\n"
            "import Qtui\n"
            "import Qtui.Widgets\n"
            "Item {\n"
            "    required property int index\n"
            "    required property string name\n"
            "    required property int age\n"
            "    property string title: name + \" (\" + age + \")\"\n"
            "}\n"
        ), QUrl());

        if (component.status() != QQmlComponent::Ready) {
            qWarning() << "Component error:" << component.errorString();
        }
        QCOMPARE(component.status(), QQmlComponent::Ready);

        // Test ListView with required properties
        ListView list;
        list.setWidth(30);
        list.setHeight(5);
        QQmlEngine::setContextForObject(&list, engine.rootContext());
        list.setDelegate(&component);
        list.setModel(QVariantList{
            QVariantMap{{"name", "Alice"}, {"age", 30}},
            QVariantMap{{"name", "Bob"}, {"age", 25}}
        });

        TextGridBuffer buffer;
        buffer.resize(30, 5);
        list.render(buffer, 0, 0);

        const QList<Item*> delegateChildren = list.findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(delegateChildren.size(), 2);
        Item* child0 = delegateChildren.at(0);
        QCOMPARE(child0->property("index").toInt(), 0);
        QCOMPARE(child0->property("name").toString(), QString("Alice"));
        QCOMPARE(child0->property("age").toInt(), 30);
        QCOMPARE(child0->property("title").toString(), QString("Alice (30)"));

        Item* child1 = delegateChildren.at(1);
        QCOMPARE(child1->property("index").toInt(), 1);
        QCOMPARE(child1->property("name").toString(), QString("Bob"));
        QCOMPARE(child1->property("age").toInt(), 25);
        QCOMPARE(child1->property("title").toString(), QString("Bob (25)"));

        // Test Repeater with required properties
        Item parentItem;
        QQmlEngine::setContextForObject(&parentItem, engine.rootContext());
        Repeater repeater;
        repeater.setParentItem(&parentItem);
        QQmlEngine::setContextForObject(&repeater, engine.rootContext());
        repeater.setDelegate(&component);
        repeater.setModel(QVariantList{
            QVariantMap{{"name", "Charlie"}, {"age", 40}}
        });

        const QList<Item*> repChildren = parentItem.findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
        Item* repChild = nullptr;
        for (Item* item : repChildren) {
            if (item != &repeater) {
                repChild = item;
                break;
            }
        }
        QVERIFY(repChild != nullptr);
        QCOMPARE(repChild->property("name").toString(), QString("Charlie"));
        QCOMPARE(repChild->property("age").toInt(), 40);

        // Test TreeView with required properties
        QQmlComponent treeComp(&engine);
        treeComp.setData(QByteArray(
            "import QtQuick\n"
            "import Qtui\n"
            "import Qtui.Widgets\n"
            "Item {\n"
            "    required property int index\n"
            "    required property string name\n"
            "    required property int depth\n"
            "}\n"
        ), QUrl());

        TreeView tree;
        QQmlEngine::setContextForObject(&tree, engine.rootContext());
        tree.setHeight(10);
        tree.setWidth(30);
        tree.setDelegate(&treeComp);
        tree.setModel(QVariantMap{
            {"name", "RootFolder"},
            {"children", QVariantList{
                QVariantMap{{"name", "ChildFolder"}}
            }}
        });
        tree.render(buffer, 0, 0);
        tree.expand(0);
        tree.render(buffer, 0, 0);

        const QList<Item*> treeChildren = tree.findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(treeChildren.size(), 2);
        Item* treeChild0 = treeChildren.at(0);
        QCOMPARE(treeChild0->property("name").toString(), QString("RootFolder"));
    }
};

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
