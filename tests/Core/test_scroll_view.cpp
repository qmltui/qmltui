#include <QTest>
#include <QObject>
#include <Qamui/Core/Items/ScrollView.h>
#include <Qamui/Core/Engine/KeyEvent.h>
#include <Qamui/Core/Engine/MouseEvent.h>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Items/MouseArea.h>

class TestScrollView;

#include <Qamui/Core/Engine/TerminalEngine.h>

using namespace qamui;

class TestScrollView : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testScrollKeys();
    void testBounds();
    void testMouseWheel();
    void testScrollHitTesting();
};

void TestScrollView::testInitialState() {
    ScrollView view;
    QCOMPARE(view.contentHeight(), 0);
    QCOMPARE(view.contentY(), 0);
}

void TestScrollView::testScrollKeys() {
    ScrollView view;
    view.setHeight(10);
    view.setContentHeight(20); // 10 lines of scroll available

    KeyEvent evDown(Qt::Key_Down, "", 0);
    view.keyPressEvent(&evDown);
    QCOMPARE(view.contentY(), 1);

    KeyEvent evUp(Qt::Key_Up, "", 0);
    view.keyPressEvent(&evUp);
    QCOMPARE(view.contentY(), 0);
}

void TestScrollView::testBounds() {
    ScrollView view;
    view.setHeight(10);
    view.setContentHeight(15); 

    view.setContentY(5); 
    QCOMPARE(view.contentY(), 5);

    KeyEvent evDown(Qt::Key_Down, "", 0);
    view.keyPressEvent(&evDown);
    // Max scroll is contentHeight (15) - height (10) = 5
    QCOMPARE(view.contentY(), 5); // shouldn't go to 6

    view.setContentY(0);
    KeyEvent evUp(Qt::Key_Up, "", 0);
    view.keyPressEvent(&evUp);
    QCOMPARE(view.contentY(), 0); // shouldn't go to -1
}

void TestScrollView::testMouseWheel() {
    ScrollView view;
    view.setHeight(10);
    view.setContentHeight(20);

    MouseEvent evUp(MouseEvent::Type::Wheel, 0, 0, 64);
    view.mousePressEvent(&evUp);
    QCOMPARE(view.contentY(), 0); // already at top

    MouseEvent evDown(MouseEvent::Type::Wheel, 0, 0, 65);
    view.mousePressEvent(&evDown);
    QCOMPARE(view.contentY(), 1);

    view.mousePressEvent(&evDown);
    QCOMPARE(view.contentY(), 2);

    view.mousePressEvent(&evUp);
    QCOMPARE(view.contentY(), 1);
}

void TestScrollView::testScrollHitTesting() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    ScrollView* scrollView = new ScrollView(&terminal);
    scrollView->setX(5);
    scrollView->setY(2);
    scrollView->setWidth(20);
    scrollView->setHeight(10);
    scrollView->setContentHeight(40);
    scrollView->setClip(true);

    MouseArea* targetMouseArea = new MouseArea(scrollView);
    targetMouseArea->setX(0);
    targetMouseArea->setY(15);
    targetMouseArea->setWidth(10);
    targetMouseArea->setHeight(3);

    bool clicked = false;
    QObject::connect(targetMouseArea, &MouseArea::clicked, [&]() {
        clicked = true;
    });

    TerminalEngine engine(&terminal);

    // 1. Click at absolute coordinates where the child would be if not clipped:
    // Screen coordinates: x = 7, y = 17 (2 + 15).
    // Because scrollview is clipped and contentY is 0, this click is outside the scrollview's viewport (y bounds 2 to 12).
    // It should not trigger targetMouseArea.
    engine.handleMouseInput(0, 7, 17, false);
    engine.handleMouseInput(0, 7, 17, true);
    QVERIFY(!clicked);

    // 2. Scroll content by 10 lines. Now targetMouseArea is inside viewport.
    // Relative child Y is 15. Offset by contentY = 10, relative visual Y is 5.
    // Screen visual Y is scrollView->y() + visual Y = 2 + 5 = 7.
    // Screen visual X is scrollView->x() + child->x() = 5 + 0 = 5.
    scrollView->setContentY(10);

    // Click at screen x = 7, y = 7 (which is inside targetMouseArea visually).
    engine.handleMouseInput(0, 7, 7, false);
    engine.handleMouseInput(0, 7, 7, true);
    QVERIFY(clicked);

    // Verify local mouse coordinates in MouseArea:
    // MouseArea absolute position is (5, 7).
    // Click coordinates are (7, 7), so local mouse coordinates should be (2, 0).
    QCOMPARE(targetMouseArea->mouseX(), 2);
    QCOMPARE(targetMouseArea->mouseY(), 0);
}

QTEST_MAIN(TestScrollView)
#include "test_scroll_view.moc"
