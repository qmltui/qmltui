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
#include <Gradient.h>
#include <GradientStop.h>
#include <Paragraph.h>
#include <Rectangle.h>
#include <TextGridBuffer.h>
#include <MouseEvent.h>
#include <KeyEvent.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QAbstractItemModel>

using namespace qtui;

class HierarchicalTestModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit HierarchicalTestModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
        m_parentNode.text = QStringLiteral("parent");
        m_parentNode.parent = &m_root;
        m_child.text = QStringLiteral("child");
        m_child.parent = &m_parentNode;
        m_grandchild.text = QStringLiteral("grandchild");
        m_grandchild.parent = &m_child;
        m_sibling.text = QStringLiteral("sibling");
        m_sibling.parent = &m_root;
        m_root.children = {&m_parentNode, &m_sibling};
        m_parentNode.children = {&m_child};
        m_child.children = {&m_grandchild};
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (column != 0 || row < 0)
            return {};
        Node *parentNode = parent.isValid()
            ? static_cast<Node *>(parent.internalPointer()) : const_cast<Node *>(&m_root);
        if (row >= parentNode->children.size())
            return {};
        return createIndex(row, column, parentNode->children.at(row));
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid())
            return {};
        Node *node = static_cast<Node *>(child.internalPointer());
        Node *parentNode = node ? node->parent : nullptr;
        if (!parentNode || parentNode == &m_root)
            return {};
        return createIndex(parentNode->parent->children.indexOf(parentNode), 0, parentNode);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.column() > 0)
            return 0;
        Node *node = parent.isValid()
            ? static_cast<Node *>(parent.internalPointer()) : const_cast<Node *>(&m_root);
        return node ? node->children.size() : 0;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};
        Node *node = static_cast<Node *>(index.internalPointer());
        return node ? QVariant(node->text) : QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::DisplayRole, QByteArrayLiteral("display")}};
    }

    void renameGrandchild(const QString &text)
    {
        m_grandchild.text = text;
        const QModelIndex changed = createIndex(0, 0, &m_grandchild);
        emit dataChanged(changed, changed, {Qt::DisplayRole});
    }

private:
    struct Node {
        QString text;
        Node *parent = nullptr;
        QList<Node *> children;
    };

    Node m_root;
    Node m_parentNode;
    Node m_child;
    Node m_grandchild;
    Node m_sibling;
};

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

        QCOMPARE(table.selectedRow(), -1);
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
    MouseEvent mouseEv(MouseEvent::Type::Press, 0, 0, 0);
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

    void testGaugeWithGradient() {
        Gauge gauge;
        gauge.setWidth(30);
        gauge.setHeight(1);
        gauge.setLabel("GPU: ");
        gauge.setValue(50.0);

        Gradient gradient;
        auto *stop1 = new GradientStop(&gradient);
        stop1->setPosition(0.0);
        stop1->setColor("#00ff00");

        auto *stop2 = new GradientStop(&gradient);
        stop2->setPosition(1.0);
        stop2->setColor("#ff0000");

        auto stopsProp = gradient.stops();
        stopsProp.append(&stopsProp, stop1);
        stopsProp.append(&stopsProp, stop2);

        gauge.setGradient(&gradient);
        QCOMPARE(gauge.gradient(), &gradient);

        TextGridBuffer buffer;
        buffer.resize(30, 1);
        gauge.render(buffer, 0, 0);

        QCOMPARE(gauge.value(), 50.0);
    }

    void testGaugeInBorderedRectangle() {
        Rectangle frame;
        frame.setWidth(30);
        frame.setHeight(3);
        frame.border()->setStyle("single");

        Gauge gauge;
        gauge.setParentItem(&frame);
        gauge.setX(1);
        gauge.setY(1);
        gauge.setWidth(28);
        gauge.setHeight(1);
        gauge.setLabel("CPU: ");
        gauge.setValue(50.0);

        TextGridBuffer buffer;
        buffer.resize(30, 3);
        frame.render(buffer, 0, 0);

        // Row 0 has top border (e.g. ┌)
        QVERIFY(buffer.cellAt(0, 0).character.startsWith("\xE2\x94\x8C"));
        // Row 1 has label and content inside the box
        QCOMPARE(buffer.cellAt(1, 1).character, QByteArray("C"));
        QCOMPARE(buffer.cellAt(2, 1).character, QByteArray("P"));
        QCOMPARE(buffer.cellAt(3, 1).character, QByteArray("U"));
        // Row 2 has bottom border (e.g. └)
        QVERIFY(buffer.cellAt(0, 2).character.startsWith("\xE2\x94\x94"));
    }

    void testBordersBelongToRectangle() {
        Rectangle rectangle;
        Paragraph paragraph;
        TableView table;
        TreeView tree;
        Gauge gauge;
        Sparkline sparkline;
        BigText bigText;
        TabHeader tabs;

        QVERIFY(rectangle.metaObject()->indexOfProperty("border") >= 0);
        const QObject *borderlessItems[] = {
            &paragraph, &table, &tree, &gauge, &sparkline, &bigText, &tabs
        };
        for (const QObject *item : borderlessItems) {
            QCOMPARE(item->metaObject()->indexOfProperty("border"), -1);
            QCOMPARE(item->metaObject()->indexOfProperty("borderStyle"), -1);
            QCOMPARE(item->metaObject()->indexOfProperty("borderColor"), -1);
        }
    }

    void testGaugeShowBrackets() {
        Gauge gauge;
        gauge.setWidth(20);
        gauge.setHeight(1);
        gauge.setLabel("");
        gauge.setValue(50.0);
        gauge.setShowPercentage(false);
        gauge.setShowBrackets(false);
        gauge.setFilledChar("=");
        gauge.setUnfilledChar("-");

        TextGridBuffer buffer;
        buffer.resize(20, 1);
        gauge.render(buffer, 0, 0);

        // Entire 20 characters should be filled or unfilled bar characters, no '[' or ']'
        for (int x = 0; x < 10; ++x) {
            QCOMPARE(buffer.cellAt(x, 0).character, QByteArray("="));
        }
        for (int x = 10; x < 20; ++x) {
            QCOMPARE(buffer.cellAt(x, 0).character, QByteArray("-"));
        }
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
            ""
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
            ""
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

    void testListViewQtQuickContract() {
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import Qtui

            ListView {
                id: view
                width: 30
                height: 5
                property string lexicalToken: "lexical-context"
                model: [
                    { name: "Alice" },
                    { name: "Bob" }
                ]
                currentIndex: -1
                delegate: Item {
                    required property int index
                    required property var model
                    required property var modelData
                    required property string name
                    property bool attachedCurrent: ListView.isCurrentItem
                    property bool attachedViewMatches: ListView.view === view
                    property string capturedToken: view.lexicalToken
                }
            }
        )", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto *list = qobject_cast<ListView *>(object.get());
        QVERIFY(list);

        TextGridBuffer buffer;
        buffer.resize(30, 5);
        list->render(buffer, 0, 0);
        QCOMPARE(list->currentIndex(), -1);

        const QList<Item *> delegates =
            list->findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(delegates.size(), 2);
        for (Item *delegate : delegates) {
            QVERIFY(delegate->property("attachedViewMatches").toBool());
            QCOMPARE(delegate->property("capturedToken").toString(),
                     QStringLiteral("lexical-context"));
            QVERIFY(!delegate->property("attachedCurrent").toBool());
            QVERIFY(delegate->metaObject()->indexOfProperty("isCurrentItem") < 0);
        }

        list->setCurrentIndex(1);
        for (Item *delegate : delegates) {
            QCOMPARE(delegate->property("attachedCurrent").toBool(),
                     delegate->property("index").toInt() == 1);
        }
        list->setCurrentIndex(-1);
        QCOMPARE(list->currentIndex(), -1);
        for (Item *delegate : delegates)
            QVERIFY(!delegate->property("attachedCurrent").toBool());
    }

    void testTableViewDelegateContracts() {
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

        QQmlComponent cellComponent(&engine);
        cellComponent.setData(R"(
            import Qtui
            Item {
                required property int index
                required property int row
                required property int column
                required property bool current
                required property bool selected
                required property var model
                required property var modelData
                required property string display
                required property string name
            }
        )", QUrl());
        QVERIFY2(cellComponent.isReady(), qPrintable(cellComponent.errorString()));

        TableView table;
        QQmlEngine::setContextForObject(&table, engine.rootContext());
        table.setWidth(30);
        table.setHeight(6);
        table.setColumns(QVariantList{
            QVariantMap{{"title", "ID"}, {"key", "id"}, {"width", 10}},
            QVariantMap{{"title", "Name"}, {"key", "name"}, {"width", 20}}
        });
        table.setModel(QVariantList{
            QVariantMap{{"id", "1"}, {"name", "Alice"}},
            QVariantMap{{"id", "2"}, {"name", "Bob"}}
        });
        table.setSelectedRow(1);
        table.setCurrentColumn(1);
        table.setDelegate(&cellComponent);

        TextGridBuffer buffer;
        buffer.resize(30, 6);
        table.render(buffer, 0, 0);

        const QList<Item *> cells =
            table.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(cells.size(), 4);
        Item *currentCell = nullptr;
        for (Item *cell : cells) {
            QCOMPARE(cell->property("index").toInt(),
                     cell->property("row").toInt() * 2
                         + cell->property("column").toInt());
            QCOMPARE(cell->property("selected").toBool(),
                     cell->property("current").toBool());
            if (cell->property("current").toBool())
                currentCell = cell;
        }
        QVERIFY(currentCell);
        QCOMPARE(currentCell->property("row").toInt(), 1);
        QCOMPARE(currentCell->property("column").toInt(), 1);
        QCOMPARE(currentCell->property("modelData").toString(),
                 QStringLiteral("Bob"));
        QCOMPARE(currentCell->property("name").toString(), QStringLiteral("Bob"));

        QQmlComponent rowComponent(&engine);
        rowComponent.setData(R"(
            import Qtui
            Item {
                required property int index
                required property int row
                required property var model
                required property var modelData
            }
        )", QUrl());
        QVERIFY2(rowComponent.isReady(), qPrintable(rowComponent.errorString()));

        table.setDelegate(nullptr);
        table.setRowDelegate(&rowComponent);
        table.render(buffer, 0, 0);
        const QList<Item *> rows =
            table.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(rows.size(), 2);
        for (Item *row : rows)
            QCOMPARE(row->property("index").toInt(), row->property("row").toInt());
    }

    void testTreeViewQtQuickContract() {
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

        QQmlComponent component(&engine);
        component.setData(R"(
            import Qtui
            Item {
                required property var treeView
                required property bool isTreeNode
                required property int row
                required property int column
                required property bool current
                required property bool selected
                required property int depth
                required property bool expanded
                required property bool hasChildren
                required property var model
                required property var modelData
                required property string display
            }
        )", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        HierarchicalTestModel model;

        TreeView tree;
        QQmlEngine::setContextForObject(&tree, engine.rootContext());
        tree.setWidth(30);
        tree.setHeight(10);
        tree.setDelegate(&component);
        tree.setModel(QVariant::fromValue(static_cast<QObject *>(&model)));
        tree.expandAll();

        TextGridBuffer buffer;
        buffer.resize(30, 10);
        tree.render(buffer, 0, 0);

        const QList<Item *> delegates =
            tree.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(delegates.size(), 4);
        int maximumDepth = -1;
        for (Item *delegate : delegates) {
            QCOMPARE(delegate->property("treeView").value<QObject *>(),
                     static_cast<QObject *>(&tree));
            QVERIFY(delegate->property("isTreeNode").toBool());
            QCOMPARE(delegate->property("column").toInt(), 0);
            QCOMPARE(delegate->property("selected").toBool(),
                     delegate->property("current").toBool());
            maximumDepth = std::max(maximumDepth,
                                    delegate->property("depth").toInt());
        }
        QCOMPARE(maximumDepth, 2);
        QCOMPARE(tree.depth(2), 2);

        QSignalSpy currentDataSpy(&tree, &TreeView::currentItemDataChanged);
        tree.setCurrentIndex(2);
        tree.render(buffer, 0, 0);
        QCOMPARE(tree.currentIndex(), 2);
        QCOMPARE(tree.currentColumn(), 0);
        QVERIFY(currentDataSpy.count() >= 1);
        const int beforeDataChange = currentDataSpy.count();
        model.renameGrandchild(QStringLiteral("renamed-grandchild"));
        tree.render(buffer, 0, 0);
        QVERIFY(currentDataSpy.count() > beforeDataChange);
        QCOMPARE(tree.currentItemData().value(QStringLiteral("display")).toString(),
                 QStringLiteral("renamed-grandchild"));

        tree.setCurrentIndex(-1);
        QCOMPARE(tree.currentIndex(), -1);
        QCOMPARE(tree.currentColumn(), -1);
    }

    void testBoundRepeaterDelegate() {
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import Qtui

            Item {
                id: root
                property string lexicalToken: "repeater-context"
                Repeater {
                    model: ["one", "two"]
                    delegate: Item {
                        required property int index
                        required property var model
                        required property var modelData
                        property string capturedToken: root.lexicalToken
                    }
                }
            }
        )", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<Item *>(object.get());
        QVERIFY(root);
        auto *repeater = root->findChild<Repeater *>();
        QVERIFY(repeater);

        int delegateCount = 0;
        for (Item *child : root->findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly)) {
            if (child == repeater)
                continue;
            QCOMPARE(child->property("capturedToken").toString(),
                     QStringLiteral("repeater-context"));
            ++delegateCount;
        }
        QCOMPARE(delegateCount, 2);
    }

    void testDestroyedModelsAreCleared() {
        {
            ListView view;
            auto *model = new HierarchicalTestModel;
            view.setModel(QVariant::fromValue(static_cast<QObject *>(model)));
            delete model;
            QVERIFY(!view.model().isValid());
        }
        {
            GridView view;
            auto *model = new HierarchicalTestModel;
            view.setModel(QVariant::fromValue(static_cast<QObject *>(model)));
            delete model;
            QVERIFY(!view.model().isValid());
        }
        {
            Repeater repeater;
            auto *model = new HierarchicalTestModel;
            repeater.setModel(QVariant::fromValue(static_cast<QObject *>(model)));
            delete model;
            QVERIFY(!repeater.model().isValid());
        }
    }
};

QTEST_MAIN(TestWidgets)
#include "test_extras.moc"
