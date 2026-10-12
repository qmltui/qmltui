#include <QtTest>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Items/Item.h>
#include <Qamui/Core/Items/MouseArea.h>
#include <Qamui/Core/Items/Popup.h>
#include <Qamui/Core/Items/ScrollView.h>
#include <Qamui/Core/Engine/InputRouter_p.h>
#include <Qamui/Core/Engine/FocusManager.h>
#include <Qamui/Core/Engine/KeyEvent.h>
#include <Qamui/Core/Engine/MouseEvent.h>
#include <Qamui/Core/Engine/Keys.h>
#include <Qamui/Core/Engine/TerminalEngine.h>

using namespace qamui;

class TestInputRouter : public QObject {
    Q_OBJECT

private slots:
    void testImplicitMouseGrab();
    void testPressArmedPopupDismissal();
    void testFocusCycling();
    void testChildMouseEventFilter();
    void testWheelDoesNotAcquireGrab();
    void testDismissalWithBackgroundMouseArea();
    void testSynchronousItemDeletionDuringDispatch();
    void testUnacceptedMouseDeletionDuringDispatch();
    void testKeyDeletionDuringDispatch();
    void testAttachedKeysDeletionDuringDispatch();
    void testHiddenAncestorRejectsFocusAndKeys();
    void testHiddenAncestorInvalidatesMouseGrab();
    void testMouseAreaClickOnlyOnReleaseInside();
    void testAcceptedButtonsAndQtButtonValues();
    void testPressedPropertyAndContainsPress();
    void testWheelSignalConsumesOnlyWhenConnected();
    void testLegacyWheelEventsNotSwallowedByMouseArea();
    void testModalPopupDiscardsStaleBackgroundGrab();
    void testTopmostPopupDismissalOrder();
    void testInlineModeMouseOffset();
};

void TestInputRouter::testImplicitMouseGrab() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea ma(&terminal);
    ma.setX(5);
    ma.setY(5);
    ma.setWidth(10);
    ma.setHeight(10);

    InputRouter router(&terminal);

    int pressCount = 0;
    int releaseCount = 0;
    int lastMouseX = -1;
    int lastMouseY = -1;

    connect(&ma, &MouseArea::pressed, [&](MouseEvent *ev) {
        pressCount++;
        lastMouseX = ev->x();
        lastMouseY = ev->y();
    });

    connect(&ma, &MouseArea::released, [&](MouseEvent *ev) {
        releaseCount++;
        lastMouseX = ev->x();
        lastMouseY = ev->y();
    });

    // 1. Press inside MouseArea at screen (7, 7) -> local (2, 2)
    router.routeMouse(0, 7, 7, false, false);
    QCOMPARE(pressCount, 1);
    QCOMPARE(lastMouseX, 2);
    QCOMPARE(lastMouseY, 2);
    QCOMPARE(router.implicitMouseGrabItem(), static_cast<Item*>(&ma));

    // 2. Drag mouse outside MouseArea to screen (25, 25) -> local (20, 20)
    // Motion button 0 (left button down with 0 in motion protocol or button 1)
    router.routeMouse(0, 25, 25, false, true);
    QCOMPARE(ma.mouseX(), 20);
    QCOMPARE(ma.mouseY(), 20);

    // 3. Release mouse at screen (30, 30) -> local (25, 25)
    router.routeMouse(0, 30, 30, true, false);
    QCOMPARE(releaseCount, 1);
    QCOMPARE(lastMouseX, 25);
    QCOMPARE(lastMouseY, 25);
    QCOMPARE(router.implicitMouseGrabItem(), nullptr);
}

void TestInputRouter::testPressArmedPopupDismissal() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    Popup popup(&terminal);
    popup.setX(10);
    popup.setY(10);
    popup.setWidth(20);
    popup.setHeight(10);
    popup.setClosePolicy(Popup::CloseOnOutsideClick);
    popup.open();
    QCOMPARE(popup.visible(), true);

    InputRouter router(&terminal);

    // Case 1: Press inside popup (15, 15) -> drag outside (5, 5) -> release outside (5, 5)
    // Should NOT close the popup because the press did not start outside.
    router.routeMouse(0, 15, 15, false, false);
    router.routeMouse(0, 5, 5, false, true);
    router.routeMouse(0, 5, 5, true, false);
    QCOMPARE(popup.visible(), true);

    // Case 2: Press outside popup (5, 5) -> release outside popup (5, 5)
    // Should close the popup
    router.routeMouse(0, 5, 5, false, false);
    router.routeMouse(0, 5, 5, true, false);
    QCOMPARE(popup.visible(), false);
}

void TestInputRouter::testFocusCycling() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    Item item1(&terminal);
    item1.setFocusable(true);
    Item item2(&terminal);
    item2.setFocusable(true);
    Item item3(&terminal);
    item3.setFocusable(true);

    InputRouter router(&terminal);

    // Initial Tab -> item1
    router.routeKey(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &item1);

    // Next Tab -> item2
    router.routeKey(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &item2);

    // Next Tab -> item3
    router.routeKey(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &item3);

    // Backtab -> item2
    router.routeKey(Qt::Key_Backtab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &item2);
}

class FilterParent : public Item {
    Q_OBJECT
public:
    explicit FilterParent(QObject* parent = nullptr) : Item(parent) {}
    bool filterCalled = false;
    Item* filteredChild = nullptr;
    MouseEvent::Type lastType = MouseEvent::Type::Move;

    bool childMouseEventFilter(Item* item, MouseEvent* event) override {
        filterCalled = true;
        filteredChild = item;
        lastType = event->type();
        event->setAccepted(true);
        return true;
    }
};

void TestInputRouter::testChildMouseEventFilter() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    FilterParent parent(&terminal);
    parent.setX(0);
    parent.setY(0);
    parent.setWidth(40);
    parent.setHeight(20);
    parent.setFiltersChildMouseEvents(true);

    MouseArea child(&parent);
    child.setX(5);
    child.setY(5);
    child.setWidth(10);
    child.setHeight(10);

    bool childPressed = false;
    connect(&child, &MouseArea::pressed, [&]() { childPressed = true; });

    InputRouter router(&terminal);

    // Mouse press inside child at (7, 7)
    router.routeMouse(0, 7, 7, false, false);

    QCOMPARE(parent.filterCalled, true);
    QCOMPARE(parent.filteredChild, static_cast<Item*>(&child));
    QCOMPARE(parent.lastType, MouseEvent::Type::Press);
    QCOMPARE(childPressed, false); // Filter intercepted and accepted event

    router.routeMouse(0, 7, 7, true, false);
    QCOMPARE(parent.lastType, MouseEvent::Type::Release);
}

void TestInputRouter::testWheelDoesNotAcquireGrab() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    ScrollView sv(&terminal);
    sv.setX(0); sv.setY(0); sv.setWidth(40); sv.setHeight(10);
    sv.setContentHeight(100);

    MouseArea ma(&terminal);
    ma.setX(50); ma.setY(15); ma.setWidth(10); ma.setHeight(5);
    int maPress = 0, maClick = 0;
    connect(&ma, &MouseArea::pressed, [&](MouseEvent*){ maPress++; });
    connect(&ma, &MouseArea::clicked, [&](MouseEvent*){ maClick++; });

    InputRouter router(&terminal);

    // Wheel event over ScrollView
    router.routeMouse(1, 5, 5, false, false, 0, 0, -1);
    QVERIFY(sv.contentY() > 0);
    QCOMPARE(router.implicitMouseGrabItem(), nullptr); // MUST NOT acquire grab

    // Next click on MouseArea must succeed immediately on the first attempt
    router.routeMouse(0, 55, 17, false, false);
    QCOMPARE(maPress, 1);
    router.routeMouse(0, 55, 17, true, false);
    QCOMPARE(maClick, 1);
}

void TestInputRouter::testDismissalWithBackgroundMouseArea() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea bg(&terminal);
    bg.setX(0); bg.setY(0); bg.setWidth(80); bg.setHeight(24);

    Popup popup(&terminal);
    popup.setX(10); popup.setY(10); popup.setWidth(20); popup.setHeight(10);
    popup.setClosePolicy(Popup::CloseOnOutsideClick);
    popup.open();
    QCOMPARE(popup.visible(), true);

    InputRouter router(&terminal);

    // Press and release outside the popup over the background MouseArea
    router.routeMouse(0, 2, 2, false, false);
    router.routeMouse(0, 2, 2, true, false);

    // Popup MUST be dismissed
    QCOMPARE(popup.visible(), false);
}

void TestInputRouter::testSynchronousItemDeletionDuringDispatch() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea* ma = new MouseArea(&terminal);
    ma->setX(0); ma->setY(0); ma->setWidth(10); ma->setHeight(5);

    InputRouter router(&terminal);
    connect(ma, &MouseArea::pressed, [&](MouseEvent*) {
        delete ma; // Synchronously deleted by handler
    });

    // Should not crash or leave dangling pointer
    router.routeMouse(0, 2, 2, false, false);
    QCOMPARE(router.implicitMouseGrabItem(), nullptr);
}

class SelfDeletingMouseItem : public Item {
public:
    using Item::Item;

    void mousePressEvent(MouseEvent *) override { delete this; }
};

void TestInputRouter::testUnacceptedMouseDeletionDuringDispatch() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 5));

    auto *item = new SelfDeletingMouseItem(&terminal);
    item->setWidth(10);
    item->setHeight(3);

    InputRouter router(&terminal);
    QPointer<Item> guard(item);
    router.routeMouse(0, 1, 1, false, false);

    QVERIFY(guard.isNull());
    QCOMPARE(router.implicitMouseGrabItem(), nullptr);
}

class SelfDeletingKeyItem : public Item {
public:
    using Item::Item;

    void keyPressEvent(KeyEvent *) override { delete this; }
};

void TestInputRouter::testKeyDeletionDuringDispatch() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 5));

    auto *item = new SelfDeletingKeyItem(&terminal);
    item->setFocusable(true);
    item->setFocus(true);

    InputRouter router(&terminal);
    QPointer<Item> guard(item);
    router.routeKey(Qt::Key_A, QStringLiteral("a"), 0);

    QVERIFY(guard.isNull());
    QCOMPARE(terminal.focusedItem(), nullptr);
}

void TestInputRouter::testAttachedKeysDeletionDuringDispatch() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 5));

    auto *item = new Item(&terminal);
    item->setFocusable(true);
    item->setFocus(true);
    auto *keys = qobject_cast<Keys *>(
        qmlAttachedPropertiesObject<Keys>(item, true));
    QVERIFY(keys);
    connect(keys, &Keys::pressed, item, [item](KeyEvent *) { delete item; });

    InputRouter router(&terminal);
    QPointer<Item> guard(item);
    router.routeKey(Qt::Key_A, QStringLiteral("a"), 0);

    QVERIFY(guard.isNull());
    QCOMPARE(terminal.focusedItem(), nullptr);
}

class CountingKeyItem : public Item {
public:
    using Item::Item;
    int keyCount = 0;

    void keyPressEvent(KeyEvent *) override { ++keyCount; }
};

void TestInputRouter::testHiddenAncestorRejectsFocusAndKeys() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 5));
    Item parent(&terminal);
    parent.setWidth(20);
    parent.setHeight(5);
    CountingKeyItem child(&parent);
    child.setFocusable(true);
    child.setWidth(5);
    child.setHeight(1);

    parent.setVisible(false);
    child.setFocus(true);
    QCOMPARE(terminal.focusedItem(), nullptr);

    parent.setVisible(true);
    child.setFocus(true);
    QCOMPARE(terminal.focusedItem(), static_cast<Item *>(&child));
    parent.setVisible(false);
    QCOMPARE(terminal.focusedItem(), nullptr);

    InputRouter router(&terminal);
    router.routeKey(Qt::Key_A, QStringLiteral("a"), 0);
    QCOMPARE(child.keyCount, 0);
}

void TestInputRouter::testHiddenAncestorInvalidatesMouseGrab() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 5));
    Item parent(&terminal);
    parent.setWidth(20);
    parent.setHeight(5);
    MouseArea child(&parent);
    child.setWidth(10);
    child.setHeight(3);

    InputRouter router(&terminal);
    router.routeMouse(0, 1, 1, false, false);
    QCOMPARE(router.implicitMouseGrabItem(), static_cast<Item *>(&child));

    parent.setVisible(false);
    router.routeMouse(0, 2, 1, false, true);
    QCOMPARE(router.implicitMouseGrabItem(), nullptr);
}

void TestInputRouter::testMouseAreaClickOnlyOnReleaseInside() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea ma(&terminal);
    ma.setX(5); ma.setY(5); ma.setWidth(10); ma.setHeight(5);

    int clicks = 0;
    int releases = 0;
    connect(&ma, &MouseArea::clicked, [&](MouseEvent*){ clicks++; });
    connect(&ma, &MouseArea::released, [&](MouseEvent*){ releases++; });

    InputRouter router(&terminal);

    // Press inside (7, 7), drag far outside (60, 20), release outside (60, 20)
    router.routeMouse(0, 7, 7, false, false);
    router.routeMouse(0, 60, 20, false, true);
    router.routeMouse(0, 60, 20, true, false);

    QCOMPARE(releases, 1);
    QCOMPARE(clicks, 0); // Click MUST NOT be emitted when released outside
}

// acceptedButtons defaults to Qt.LeftButton, as in Qt Quick; other buttons
// leave the event for items behind, and delivered events carry Qt values.
void TestInputRouter::testAcceptedButtonsAndQtButtonValues() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea behind(&terminal);
    behind.setWidth(20); behind.setHeight(10);
    behind.setAcceptedButtons(Qt::RightButton);
    Qt::MouseButton behindButton = Qt::NoButton;
    connect(&behind, &MouseArea::pressed, [&](MouseEvent *ev) { behindButton = ev->button(); });

    MouseArea front(&behind);
    front.setWidth(20); front.setHeight(10);
    Qt::MouseButton frontButton = Qt::NoButton;
    connect(&front, &MouseArea::pressed, [&](MouseEvent *ev) { frontButton = ev->button(); });

    InputRouter router(&terminal);

    // Native 2 is the right button: the front area (left only) lets it
    // through to the right-accepting area behind it.
    router.routeMouse(2, 5, 5, false, false);
    QCOMPARE(frontButton, Qt::NoButton);
    QCOMPARE(behindButton, Qt::RightButton);
    router.routeMouse(2, 5, 5, true, false);

    // Native 0 is the left button and stops at the front area.
    router.routeMouse(0, 5, 5, false, false);
    QCOMPARE(frontButton, Qt::LeftButton);
    router.routeMouse(0, 5, 5, true, false);
}

void TestInputRouter::testPressedPropertyAndContainsPress() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea ma(&terminal);
    ma.setX(5); ma.setY(5); ma.setWidth(10); ma.setHeight(5);

    InputRouter router(&terminal);
    QCOMPARE(ma.isPressed(), false);

    router.routeMouse(0, 7, 7, false, false);
    QCOMPARE(ma.isPressed(), true);
    QCOMPARE(ma.pressedButtons(), Qt::MouseButtons(Qt::LeftButton));
    QCOMPARE(ma.containsPress(), true);

    // Button-held motion out of the area: still pressed, no longer inside.
    router.routeMouse(0, 30, 20, false, true);
    QCOMPARE(ma.isPressed(), true);
    QCOMPARE(ma.containsPress(), false);

    // Back inside, then release: everything ends.
    router.routeMouse(0, 8, 6, false, true);
    QCOMPARE(ma.containsPress(), true);
    router.routeMouse(0, 8, 6, true, false);
    QCOMPARE(ma.isPressed(), false);
    QCOMPARE(ma.containsPress(), false);

    // Hiding a pressed area ends the press: no release will be delivered.
    router.routeMouse(0, 7, 7, false, false);
    QCOMPARE(ma.isPressed(), true);
    ma.setVisible(false);
    QCOMPARE(ma.isPressed(), false);
    QCOMPARE(ma.containsPress(), false);
}

void TestInputRouter::testWheelSignalConsumesOnlyWhenConnected() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(40, 10));

    ScrollView* view = new ScrollView(&terminal);
    view->setWidth(20); view->setHeight(5);
    view->setContentHeight(50);

    MouseArea* area = new MouseArea(view);
    area->setWidth(20); area->setHeight(5);

    InputRouter router(&terminal);

    // A handler takes the wheel; the view behind must not scroll.
    int wheelDeltaY = 0;
    connect(area, &MouseArea::wheel, [&](MouseEvent *ev) { wheelDeltaY = ev->wheelDeltaY(); });
    router.routeMouse(1, 2, 2, false, false, 0, 0, -1);
    QCOMPARE(wheelDeltaY, -1);
    QCOMPARE(view->contentY(), 0);
}

void TestInputRouter::testLegacyWheelEventsNotSwallowedByMouseArea() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(40, 10));

    ScrollView* view = new ScrollView(&terminal);
    view->setWidth(20); view->setHeight(5);
    view->setContentHeight(50);

    MouseArea* area = new MouseArea(view);
    area->setWidth(20); area->setHeight(5);

    InputRouter router(&terminal);

    // Legacy wheel-down sent as button 65 press over the MouseArea
    router.routeMouse(1, 2, 2, false, false, 0, 0, -1);
    QVERIFY(view->contentY() > 0);
}

void TestInputRouter::testModalPopupDiscardsStaleBackgroundGrab() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea bg(&terminal);
    bg.setX(0); bg.setY(0); bg.setWidth(40); bg.setHeight(10);
    int bgPress = 0;
    connect(&bg, &MouseArea::pressed, [&](MouseEvent*){ bgPress++; });

    Popup modal(&terminal);
    modal.setX(20); modal.setY(12); modal.setWidth(30); modal.setHeight(8);
    modal.setModal(true);
    modal.setClosePolicy(Popup::NoAutoClose);

    InputRouter router(&terminal);

    // Press on bg to take grab
    router.routeMouse(0, 5, 5, false, false);
    QCOMPARE(router.implicitMouseGrabItem(), static_cast<Item*>(&bg));

    // Modal popup opens
    modal.open();

    // Now click outside modal popup -> stale grab on bg MUST NOT deliver press to bg
    bgPress = 0;
    router.routeMouse(0, 60, 3, false, false);
    QCOMPARE(bgPress, 0);
    QCOMPARE(router.implicitMouseGrabItem(), nullptr);
}

void TestInputRouter::testTopmostPopupDismissalOrder() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    Popup low(&terminal);
    low.setX(10); low.setY(10); low.setWidth(10); low.setHeight(5);
    low.setClosePolicy(Popup::CloseOnOutsideClick);
    low.open();

    Popup high(&terminal);
    high.setX(40); high.setY(10); high.setWidth(10); high.setHeight(5);
    high.setClosePolicy(Popup::CloseOnOutsideClick);
    high.open();

    InputRouter router(&terminal);

    // Outside click dismisses TOPMOST popup first
    router.routeMouse(0, 1, 1, false, false);
    router.routeMouse(0, 1, 1, true, false);

    QCOMPARE(high.visible(), false);
    QCOMPARE(low.visible(), true);
}

void TestInputRouter::testInlineModeMouseOffset() {
    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    terminal.engine()->setTerminalOutputEnabled(false);
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    MouseArea ma(&terminal);
    ma.setX(0);
    ma.setY(0);
    ma.setWidth(40);
    ma.setHeight(4);
    ma.setHoverEnabled(true);

    int pressCount = 0;
    int clickLocalX = -1;
    int clickLocalY = -1;
    connect(&ma, &MouseArea::pressed, [&](MouseEvent *ev) {
        pressCount++;
        clickLocalX = ev->x();
        clickLocalY = ev->y();
    });

    QTRY_COMPARE(terminal.engine()->currentFrame()->height(), 4);
    InputRouter router(&terminal);

    // Terminal height is 24, inline prompt height is 4.
    // The prompt region sits at terminal rows [19, 22].
    // 1. Mouse motion in scrollback above prompt at screen (10, 5):
    // Should NOT hover MouseArea.
    router.routeMouse(0, 10, 5, false, true);
    QCOMPARE(terminal.hoveredItem(), nullptr);
    QCOMPARE(ma.containsMouse(), false);

    // 2. Mouse motion inside prompt at screen (10, 20):
    // Local row should be 20 - 19 = 1.
    router.routeMouse(0, 10, 20, false, true);
    QCOMPARE(terminal.hoveredItem(), static_cast<Item*>(&ma));
    QCOMPARE(ma.containsMouse(), true);
    QCOMPARE(ma.mouseY(), 1);
    QCOMPARE(ma.mouseX(), 10);

    // 3. Mouse press at screen (10, 21):
    // Local row should be 21 - 19 = 2.
    router.routeMouse(0, 10, 21, false, false);
    QCOMPARE(pressCount, 1);
    QCOMPARE(clickLocalX, 10);
    QCOMPARE(clickLocalY, 2);

    // 4. Mouse release at screen (10, 21):
    router.routeMouse(0, 10, 21, true, false);

    // 5. Test modal popup in inline mode:
    Popup popup(&terminal);
    popup.setX(0);
    popup.setY(0);
    popup.setWidth(40);
    popup.setHeight(10);
    popup.setModal(true);

    MouseArea popupMa(&popup);
    popupMa.setX(0);
    popupMa.setY(0);
    popupMa.setWidth(40);
    popupMa.setHeight(10);
    popupMa.setHoverEnabled(true);

    popup.open();
    QTRY_COMPARE(terminal.engine()->currentFrame()->height(), 10);
    // Now prompt height is 10. Start row is 24 - 1 - 10 = 13.
    // Modal sits at screen rows [13, 22].
    // Motion at screen row 13 -> local row 0 in popup.
    router.routeMouse(0, 5, 13, false, true);
    QCOMPARE(terminal.hoveredItem(), static_cast<Item*>(&popupMa));
    QCOMPARE(popupMa.mouseY(), 0);

    // Motion at screen row 22 -> local row 9 in popup.
    router.routeMouse(0, 5, 22, false, true);
    QCOMPARE(terminal.hoveredItem(), static_cast<Item*>(&popupMa));
    QCOMPARE(popupMa.mouseY(), 9);

    // Motion at screen row 10 (above modal) -> modal not hovered!
    router.routeMouse(0, 5, 10, false, true);
    QCOMPARE(terminal.hoveredItem(), nullptr);
    QCOMPARE(popupMa.containsMouse(), false);

    // Content beyond the committed buffer is not clickable, even if the
    // popup's own geometry extends into the reserved trailing cursor row.
    popup.setHeight(30);
    popupMa.setHeight(30);
    QTRY_COMPARE(terminal.engine()->currentFrame()->height(), 23);
    QSignalSpy popupPresses(&popupMa, &MouseArea::pressed);
    router.routeMouse(0, 5, 23, false, false);
    QCOMPARE(popupPresses.count(), 0);
}

QTEST_MAIN(TestInputRouter)
#include "test_input_router.moc"
