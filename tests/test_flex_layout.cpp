#include <QTest>
#include <QObject>
#include "FlexLayout.h"
#include "Text.h"

using namespace qtui;

class TestFlexLayout : public QObject {
    Q_OBJECT

private slots:
    void testBasicRowLayout();
    void testRowGrowDistribution();
    void testColumnLayout();
    void testFlexWrap();
};

void TestFlexLayout::testBasicRowLayout() {
    FlexLayout layout;
    layout.setWidth(20);
    layout.setHeight(5);
    layout.setFlexDirection(FlexLayout::Row);

    Item* child1 = new Item(&layout);
    child1->setWidth(5);
    child1->setHeight(2);

    Item* child2 = new Item(&layout);
    child2->setWidth(8);
    child2->setHeight(3);

    // Call render once to force layout pass
    TextGridBuffer buffer;
    buffer.resize(20, 5);
    layout.render(buffer, 0, 0);

    // Verify Row layout positioning
    QCOMPARE(child1->x(), 0);
    QCOMPARE(child2->x(), 5);
}

void TestFlexLayout::testRowGrowDistribution() {
    FlexLayout layout;
    layout.setWidth(20);
    layout.setHeight(5);
    layout.setFlexDirection(FlexLayout::Row);

    Item* child1 = new Item(&layout);
    child1->setFlexGrow(1.0);
    child1->setFlexBasis(5);

    Item* child2 = new Item(&layout);
    child2->setFlexGrow(2.0);
    child2->setFlexBasis(5);

    TextGridBuffer buffer;
    buffer.resize(20, 5);
    layout.render(buffer, 0, 0);

    // Total available: 20
    // Sum basis: 5 + 5 = 10
    // Remaining space: 20 - 10 = 10
    // Grow distribution:
    // child1 gets: 5 + (1 / 3) * 10 = 8
    // child2 gets: 5 + (2 / 3) * 10 = 11
    // Total sizes should equal width (with slight integer rounding):
    QCOMPARE(child1->width(), 8);
    QCOMPARE(child2->width(), 11);
    QCOMPARE(child1->x(), 0);
    QCOMPARE(child2->x(), 8);
}

void TestFlexLayout::testColumnLayout() {
    FlexLayout layout;
    layout.setWidth(10);
    layout.setHeight(15);
    layout.setFlexDirection(FlexLayout::Column);

    Item* child1 = new Item(&layout);
    child1->setWidth(4);
    child1->setHeight(3);

    Item* child2 = new Item(&layout);
    child2->setWidth(5);
    child2->setHeight(4);

    TextGridBuffer buffer;
    buffer.resize(10, 15);
    layout.render(buffer, 0, 0);

    // Verify Column layout positioning
    QCOMPARE(child1->y(), 0);
    QCOMPARE(child2->y(), 3);
}

void TestFlexLayout::testFlexWrap() {
    FlexLayout layout;
    layout.setWidth(10);
    layout.setHeight(10);
    layout.setFlexDirection(FlexLayout::Row);
    layout.setFlexWrap(FlexLayout::Wrap);

    Item* child1 = new Item(&layout);
    child1->setWidth(6);
    child1->setHeight(2);

    Item* child2 = new Item(&layout);
    child2->setWidth(6);
    child2->setHeight(3);

    TextGridBuffer buffer;
    buffer.resize(10, 10);
    layout.render(buffer, 0, 0);

    // Child1 and Child2 cannot fit in one line (6 + 6 = 12 > 10)
    // Child2 must wrap to the next line
    QCOMPARE(child1->x(), 0);
    QCOMPARE(child1->y(), 0);

    QCOMPARE(child2->x(), 0);
    QCOMPARE(child2->y(), 2); // shifted down by child1's line cross size (height 2)
}

QTEST_MAIN(TestFlexLayout)
#include "test_flex_layout.moc"
