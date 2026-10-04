#include <QTest>
#include <QObject>
#include <QSignalSpy>
#include "Row.h"
#include "Column.h"
#include "GridLayout.h"
#include "Layout.h"
#include "ScrollView.h"
#include "Text.h"
#include "TextGridBuffer.h"

using namespace qtui;

class TestLayout : public QObject {
    Q_OBJECT

private slots:
    void testRowLayout();
    void testColumnLayout();
    void testGridLayout();
    void testDirtyFlagsBubbling();
    void testAnchorInvalidationWithoutRender();
    void testContainerRelayoutOnMutation();
    void testScrollViewDirtyEmissionNotSink();
    void testAnchorImplicitTargetResizing();
};

void TestLayout::testRowLayout() {
    Row row;
    row.setSpacing(2);
    row.setPadding(1);
    
    Item* child1 = new Item();
    child1->setParentItem(&row);
    child1->setWidth(4);
    child1->setHeight(2);
    
    Item* child2 = new Item();
    child2->setParentItem(&row);
    child2->setWidth(5);
    child2->setHeight(2);

    TextGridBuffer buffer;
    row.render(buffer, 0, 0);

    QCOMPARE(child1->x(), 1); // left padding
    QCOMPARE(child1->y(), 1); // top padding
    QCOMPARE(child2->x(), 1 + 4 + 2); // left pad + child1 width + spacing
    QCOMPARE(child2->y(), 1);

    QCOMPARE(row.implicitWidth(), 1 + 1 + 4 + 5 + 2); // left + right + w1 + w2 + spacing
    QCOMPARE(row.implicitHeight(), 1 + 1 + 2); // top + bottom + max_height
}

void TestLayout::testColumnLayout() {
    Column col;
    col.setSpacing(1);
    col.setPadding(2);
    
    Item* child1 = new Item();
    child1->setParentItem(&col);
    child1->setWidth(4);
    child1->setHeight(3);
    
    Item* child2 = new Item();
    child2->setParentItem(&col);
    child2->setWidth(5);
    child2->setHeight(4);

    TextGridBuffer buffer;
    col.render(buffer, 0, 0);

    QCOMPARE(child1->x(), 2); // left padding
    QCOMPARE(child1->y(), 2); // top padding
    QCOMPARE(child2->x(), 2); 
    QCOMPARE(child2->y(), 2 + 3 + 1); // top pad + child1 height + spacing

    QCOMPARE(col.implicitHeight(), 2 + 2 + 3 + 4 + 1); // top + bottom + h1 + h2 + spacing
    QCOMPARE(col.implicitWidth(), 2 + 2 + 5); // left + right + max_width
}

void TestLayout::testGridLayout() {
    GridLayout grid;
    grid.setColumns(2);
    grid.setColumnSpacing(2);
    grid.setRowSpacing(1);

    Item *label1 = new Item(&grid);
    label1->setImplicitWidth(10);
    label1->setImplicitHeight(1);

    Item *field1 = new Item(&grid);
    field1->setImplicitWidth(20);
    field1->setImplicitHeight(3);

    Item *label2 = new Item(&grid);
    label2->setImplicitWidth(10);
    label2->setImplicitHeight(1);

    Item *field2 = new Item(&grid);
    field2->setImplicitWidth(20);
    field2->setImplicitHeight(3);

    TextGridBuffer buffer;
    grid.render(buffer, 0, 0);

    QCOMPARE(label1->x(), 0);
    QCOMPARE(field1->x(), 12);
    QCOMPARE(label2->x(), 0);
    QCOMPARE(field2->x(), 12);

    QCOMPARE(label1->y(), 0);
    QCOMPARE(field1->y(), 0);
    QCOMPARE(label2->y(), 4);
    QCOMPARE(field2->y(), 4);
}

void TestLayout::testDirtyFlagsBubbling() {
    Item root;
    Column col;
    col.setParentItem(&root);

    Item child;
    child.setParentItem(&col);

    root.clearDirty();
    col.clearDirty();
    child.clearDirty();

    QCOMPARE(root.dirtyFlags().testFlag(Item::DirtyFlag::DescendantDirty), false);
    QCOMPARE(col.dirtyFlags().testFlag(Item::DirtyFlag::DescendantDirty), false);
    QCOMPARE(child.dirtyFlags().testFlag(Item::DirtyFlag::SelfDirty), false);

    // Mutate child
    child.setWidth(30);

    QCOMPARE(child.dirtyFlags().testFlag(Item::DirtyFlag::SelfDirty), true);
    QCOMPARE(col.dirtyFlags().testFlag(Item::DirtyFlag::DescendantDirty), true);
    QCOMPARE(root.dirtyFlags().testFlag(Item::DirtyFlag::DescendantDirty), true);

    // After rendering root, flags should be cleared
    TextGridBuffer buffer;
    root.render(buffer, 0, 0);

    QCOMPARE(root.dirtyFlags(), Item::DirtyFlags(Item::DirtyFlag::None));
}

void TestLayout::testAnchorInvalidationWithoutRender() {
    Item parent;
    parent.setWidth(100);
    parent.setHeight(50);

    Item child;
    child.setParentItem(&parent);
    child.anchors()->setFill(&parent);

    // Anchors resolve immediately on target geometry changes
    QCOMPARE(child.width(), 100);
    QCOMPARE(child.height(), 50);

    parent.setWidth(120);
    QCOMPARE(child.width(), 120);

    parent.setHeight(80);
    QCOMPARE(child.height(), 80);
}

void TestLayout::testContainerRelayoutOnMutation() {
    TextGridBuffer buffer;
    buffer.resize(40, 20);

    Column col;
    col.setWidth(20);
    col.setHeight(20);

    Item* a = new Item(&col);
    a->setWidth(5); a->setHeight(2);
    Item* b = new Item(&col);
    b->setWidth(5); b->setHeight(2);

    col.render(buffer, 0, 0);
    QCOMPARE(a->y(), 0);
    QCOMPARE(b->y(), 2);

    // 1. Grow child a: b should re-layout and move down to y=4
    a->setHeight(4);
    col.render(buffer, 0, 0);
    QCOMPARE(a->y(), 0);
    QCOMPARE(b->y(), 4);

    // 2. Add child c: c should be laid out at y=6
    Item* c = new Item(&col);
    c->setWidth(5); c->setHeight(2);
    col.render(buffer, 0, 0);
    QCOMPARE(c->y(), 6);

    // 3. Hide child a: b should move up to y=0, c to y=2
    a->setVisible(false);
    col.render(buffer, 0, 0);
    QCOMPARE(b->y(), 0);
    QCOMPARE(c->y(), 2);
}

void TestLayout::testScrollViewDirtyEmissionNotSink() {
    TextGridBuffer buffer;
    buffer.resize(60, 20);

    Item root;
    root.setWidth(60);
    root.setHeight(20);

    ScrollView* view = new ScrollView(&root);
    view->setWidth(20);
    view->setHeight(5);
    view->setContentHeight(40);

    Text* row = new Text(view);
    row->setText("row 0");

    QSignalSpy rootDirty(&root, &Item::dirty);

    root.render(buffer, 0, 0);

    // Mutate text multiple times across frames: each mutation MUST emit dirty on root!
    for (int i = 1; i <= 3; ++i) {
        root.render(buffer, 0, 0);
        rootDirty.clear();
        row->setText(QString("row %1").arg(i));
        QVERIFY(rootDirty.count() > 0);
    }
}

void TestLayout::testAnchorImplicitTargetResizing() {
    TextGridBuffer buffer;
    buffer.resize(60, 20);

    Item root;
    root.setWidth(40);
    root.setHeight(10);

    Row* row = new Row(&root);
    Item* a = new Item(row);
    a->setWidth(5); a->setHeight(2);

    Item* overlay = new Item(&root);
    overlay->anchors()->setFill(row);

    root.render(buffer, 0, 0);
    QCOMPARE(overlay->width(), row->implicitWidth());

    // Add another item to row: row's implicit width grows to 10
    Item* b = new Item(row);
    b->setWidth(5); b->setHeight(2);

    root.render(buffer, 0, 0);
    QCOMPARE(overlay->width(), row->implicitWidth());
}

QTEST_MAIN(TestLayout)
#include "test_layout.moc"
