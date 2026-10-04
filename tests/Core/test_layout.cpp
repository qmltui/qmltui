#include <QTest>
#include <QObject>
#include "Row.h"
#include "Column.h"
#include "GridLayout.h"
#include "Layout.h"
#include "TextGridBuffer.h"

using namespace qtui;

class TestLayout : public QObject {
    Q_OBJECT

private slots:
    void testRowLayout();
    void testColumnLayout();
    void testGridLayout();
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

QTEST_MAIN(TestLayout)
#include "test_layout.moc"
