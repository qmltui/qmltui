#include <QTest>
#include <QObject>
#include <QmlTui/Core/Items/ScrollView.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/MouseArea.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include "../TestPaint.h"

using namespace qmltui;
using qmltui::testing::paintItem;

class TestScrollView : public QObject {
    Q_OBJECT

private slots:
    void testScrollKeys();
    void testBounds();
    void testMouseWheel();
    void testScrollHitTesting();
};

void TestScrollView::testScrollKeys() {
    ScrollView view;
    QCOMPARE(view.contentHeight(), 0);
    QCOMPARE(view.contentY(), 0);

    view.setWidth(10);
    view.setHeight(10);
    view.setContentHeight(20); // 10 lines of scroll available

    Text line(&view);
    line.setY(2);
    line.setWidth(10);
    line.setHeight(1);
    line.setText(QStringLiteral("Row2"));

    TextGridBuffer buffer;
    buffer.resize(10, 10);
    paintItem(view, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 2).character().toByteArray(), QByteArray("R"));
    QCOMPARE(buffer.cellAt(3, 2).character().toByteArray(), QByteArray("2"));

    KeyEvent evDown(Qt::Key_Down, "", 0);
    view.keyPressEvent(&evDown);
    QCOMPARE(view.contentY(), 1);
    buffer.clear();
    paintItem(view, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 1).character().toByteArray(), QByteArray("R"));
    QCOMPARE(buffer.cellAt(3, 1).character().toByteArray(), QByteArray("2"));
    QCOMPARE(buffer.cellAt(0, 2).character().toByteArray(), QByteArray(" "));

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

    MouseEvent evUp(MouseEvent::Type::Wheel, 0, 0, 64, 0, 0, 1);
    view.wheelEvent(&evUp);
    QCOMPARE(view.contentY(), 0); // already at top

    MouseEvent evDown(MouseEvent::Type::Wheel, 0, 0, 65, 0, 0, -1);
    view.wheelEvent(&evDown);
    QCOMPARE(view.contentY(), 1);

    view.wheelEvent(&evDown);
    QCOMPARE(view.contentY(), 2);

    view.wheelEvent(&evUp);
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
    QObject::connect(targetMouseArea, &MouseArea::clicked, [&]() { clicked = true; });

    TerminalEngine engine(&terminal);

    // 1. Click at absolute coordinates where the child would be if not clipped:
    // Screen coordinates: x = 7, y = 17 (2 + 15).
    // Because scrollview is clipped and contentY is 0, this click is outside the scrollview's
    // viewport (y bounds 2 to 12). It should not trigger targetMouseArea.
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
