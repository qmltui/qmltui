#include <QTest>
#include <QObject>
#include "ScrollView.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

using namespace qtui;

class TestScrollView : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testScrollKeys();
    void testBounds();
    void testMouseWheel();
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

    MouseEvent evUp(0, 0, 64);
    view.mousePressEvent(&evUp);
    QCOMPARE(view.contentY(), 0); // already at top

    MouseEvent evDown(0, 0, 65);
    view.mousePressEvent(&evDown);
    QCOMPARE(view.contentY(), 1);

    view.mousePressEvent(&evDown);
    QCOMPARE(view.contentY(), 2);

    view.mousePressEvent(&evUp);
    QCOMPARE(view.contentY(), 1);
}

QTEST_MAIN(TestScrollView)
#include "test_scroll_view.moc"
