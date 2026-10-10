#include <QTest>
#include <QObject>
#include <QSocketNotifier>
#include <QSignalSpy>
#include <QQmlComponent>
#include <QQmlEngine>
#include <sstream>
#include <chrono>
#include <vector>
#include <iostream>
#include <memory>

#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/items/Popup.h>
#include <Qtui/Core/items/MouseArea.h>
#include <Qtui/Core/engine/KeyEvent.h>
#include <Qtui/Core/engine/MouseEvent.h>

#include <Qtui/Core/engine/TerminalEngine.h>
#undef private

using namespace qtui;

class TestPopup : public QObject {
    Q_OBJECT

private slots:
    void testPopupRegistration();
    void testModalInputBlocking();
    void testClosePolicies();
    void testBaseVisibilityDispatchAndParentNotification();
    void testDynamicZAndPopupOrder();
    void testOwnedPopupCascade();
    void testAttachedToolTipOnFocus();
    void testCloseWhenNotOpenKeepsFocus();
    void testCloseWithoutTakingFocusLeavesFocusAlone();
    void testModalRestoresFocusClearedDuringOpen();
    void testAttachedToolTipFollowsTarget();
};

void TestPopup::testPopupRegistration() {
    Terminal terminal;
    Popup* popup = new Popup(&terminal);

    QCOMPARE(terminal.activePopups().count(), 0);

    popup->open();
    QCOMPARE(terminal.activePopups().count(), 1);
    QCOMPARE(terminal.activePopups().first(), popup);

    popup->close();
    QCOMPARE(terminal.activePopups().count(), 0);
}

void TestPopup::testModalInputBlocking() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(80, 24));

    // Add a normal mouse area in the background
    MouseArea* bgMouseArea = new MouseArea(&terminal);
    bgMouseArea->setX(0);
    bgMouseArea->setY(0);
    bgMouseArea->setWidth(10);
    bgMouseArea->setHeight(10);
    
    bool bgClicked = false;
    connect(bgMouseArea, &MouseArea::clicked, [&]() {
        bgClicked = true;
    });

    // Add a modal popup on top
    Popup* popup = new Popup(&terminal);
    popup->setX(20);
    popup->setY(5);
    popup->setWidth(15);
    popup->setHeight(5);
    popup->setModal(true);

    MouseArea* popupMouseArea = new MouseArea(popup);
    popupMouseArea->setX(0);
    popupMouseArea->setY(0);
    popupMouseArea->setWidth(5);
    popupMouseArea->setHeight(2);
    
    bool popupClicked = false;
    connect(popupMouseArea, &MouseArea::clicked, [&]() {
        popupClicked = true;
    });

    // Instantiate engine (m_notifier will be null/skipped if stdin is not raw, but constructor works)
    TerminalEngine engine(&terminal);
    
    // Open popup
    popup->open();

    // 1. Simulate mouse click inside popup MouseArea (at absolute 20, 5)
    engine.handleMouseInput(0, 21, 6, false); // press
    engine.handleMouseInput(0, 21, 6, true);  // release

    QVERIFY(popupClicked);
    QVERIFY(!bgClicked); // modal blocks background

    // Reset
    popupClicked = false;
    bgClicked = false;

    // 2. Simulate mouse click outside popup (at 2, 2)
    // Click outside should close the popup due to CloseOnOutsideClick policy (default)
    // and should NOT trigger the background click (since it's blocked by the modal popup)
    engine.handleMouseInput(0, 2, 2, false);
    engine.handleMouseInput(0, 2, 2, true);

    QVERIFY(!popupClicked);
    QVERIFY(!bgClicked);
    QVERIFY(!popup->visible()); // closed!
}

void TestPopup::testClosePolicies() {
    Terminal terminal;
    Popup* popup = new Popup(&terminal);
    popup->setModal(true);
    popup->open();

    TerminalEngine engine(&terminal);

    // Test Escape key closes popup
    QVERIFY(popup->visible());
    engine.handleKeyInput(Qt::Key_Escape, "", 0);
    QVERIFY(!popup->visible()); // closed!

    // Reset
    popup->setClosePolicy(Popup::NoAutoClose);
    popup->open();

    // Escape should NOT close it now
    engine.handleKeyInput(Qt::Key_Escape, "", 0);
    QVERIFY(popup->visible());
}

void TestPopup::testBaseVisibilityDispatchAndParentNotification() {
    Terminal firstTerminal;
    Terminal secondTerminal;
    Popup popup(&firstTerminal);
    QSignalSpy parentSpy(&popup, &Item::parentChanged);

    Item *base = &popup;
    base->setVisible(true);
    QCOMPARE(firstTerminal.activePopups().size(), 1);

    base->setParentItem(&secondTerminal);
    QCOMPARE(parentSpy.size(), 1);
    QCOMPARE(firstTerminal.activePopups().size(), 0);
    QCOMPARE(secondTerminal.activePopups().size(), 1);

    base->setVisible(false);
    QCOMPARE(secondTerminal.activePopups().size(), 0);
}

void TestPopup::testDynamicZAndPopupOrder() {
    Terminal terminal;
    Item low(&terminal);
    Item high(&terminal);
    high.setZ(5.0);
    QCOMPARE(terminal.stackingChildren().last(), &high);
    low.setZ(10.0);
    QCOMPARE(terminal.stackingChildren().last(), &low);

    Popup first(&terminal);
    Popup second(&terminal);
    first.open();
    second.open();
    QCOMPARE(terminal.orderedPopups().last(), &second);
    first.open();
    QCOMPARE(terminal.orderedPopups().last(), &first);
    second.setZ(2.0);
    QCOMPARE(terminal.orderedPopups().last(), &second);
}

void TestPopup::testOwnedPopupCascade() {
    Terminal terminal;
    Popup owner(&terminal);
    owner.setModal(true);
    Popup child(&terminal);
    child.setOwner(&owner);
    Popup unrelated(&terminal);
    unrelated.setZ(100);
    owner.open();
    unrelated.open();
    child.open();
    QCOMPARE(child.parentPopup(), &owner);
    QVERIFY(child.isDescendantOfPopup(&owner));
    QCOMPARE(terminal.orderedPopups().last(), &child);
    owner.close();
    QVERIFY(!owner.visible());
    QVERIFY(!child.visible());
}

void TestPopup::testAttachedToolTipOnFocus() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        import Qtui.Controls
        Terminal {
            width: 40; height: 10
            Item {
                id: target
                objectName: "target"
                width: 4; height: 1; focusable: true
                ToolTip.text: "Helpful"
                ToolTip.delay: 0
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *terminal = qobject_cast<Terminal*>(object.get());
    QVERIFY(terminal);
    QVERIFY(terminal->hasMouseMotionRequests());
    auto *target = terminal ? terminal->findChild<Item*>(QStringLiteral("target")) : nullptr;
    QVERIFY(target);
    terminal->setFocusedItem(target);
    QTRY_COMPARE(terminal->activePopups().size(), 1);
    QCOMPARE(terminal->activePopups().first()->owner(), target);
    delete target;
    QTRY_COMPARE(terminal->activePopups().size(), 0);
    QTRY_VERIFY(!terminal->hasMouseMotionRequests());
}

void TestPopup::testCloseWhenNotOpenKeepsFocus() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(40, 10));
    Item* field = new Item(&terminal);
    field->setWidth(4);
    field->setHeight(1);
    field->setFocusable(true);
    terminal.setFocusedItem(field);
    QCOMPARE(terminal.focusedItem(), field);

    Popup* popup = new Popup(&terminal);
    popup->setWidth(10);
    popup->setHeight(3);
    popup->close();                       // never opened: nothing to restore
    QCOMPARE(terminal.focusedItem(), field);

    popup->open();
    popup->close();
    QCOMPARE(terminal.focusedItem(), field);   // restored once
    popup->close();                            // second close is a no-op
    QCOMPARE(terminal.focusedItem(), field);
}

// A popup with nothing to focus (a tooltip) must not touch focus: neither
// when it opens nor when it closes, even if focus moved while it was open.
void TestPopup::testCloseWithoutTakingFocusLeavesFocusAlone() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(40, 10));
    Item* first = new Item(&terminal);
    first->setFocusable(true);
    Item* second = new Item(&terminal);
    second->setFocusable(true);
    terminal.setFocusedItem(first);

    Popup* popup = new Popup(&terminal);
    popup->setModal(false);
    popup->open();
    QCOMPARE(terminal.focusedItem(), first);    // opening took nothing

    terminal.setFocusedItem(second);
    popup->close();
    QCOMPARE(terminal.focusedItem(), second);   // closing must not yank it back
}

// An application binding reacting to the popup's `visible` may clear the
// focused item during open(); close() must still restore the pre-open focus.
void TestPopup::testModalRestoresFocusClearedDuringOpen() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(40, 10));
    Item* field = new Item(&terminal);
    field->setFocusable(true);
    terminal.setFocusedItem(field);

    Popup* popup = new Popup(&terminal);
    popup->setModal(true);
    connect(popup, &Item::visibleChanged, &terminal, [&]() {
        if (popup->visible())
            terminal.setFocusedItem(nullptr);
    });
    popup->open();
    QCOMPARE(terminal.focusedItem(), nullptr);
    popup->close();
    QCOMPARE(terminal.focusedItem(), field);
}

// The tip must follow its target: a pinned tip opens before Row/Column have
// placed the target, and the target may move while the tip is visible.
void TestPopup::testAttachedToolTipFollowsTarget() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        import Qtui.Controls
        Terminal {
            width: 40; height: 10
            Item {
                id: target
                objectName: "target"
                x: 5; y: 2; width: 4; height: 1
                ToolTip.text: "Tip"
                ToolTip.visible: true
                ToolTip.timeout: 0
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *terminal = qobject_cast<Terminal*>(object.get());
    QVERIFY(terminal);
    terminal->engine()->setTestViewportSize(QSize(40, 10));
    auto *target = terminal->findChild<Item*>(QStringLiteral("target"));
    QVERIFY(target);
    QTRY_COMPARE(terminal->activePopups().size(), 1);
    Popup *tip = terminal->activePopups().first();
    QCOMPARE(tip->x(), 5);
    QCOMPARE(tip->y(), 3);

    target->setX(20);
    target->setY(6);
    QCOMPARE(tip->x(), 20);
    QCOMPARE(tip->y(), 7);
}

QTEST_MAIN(TestPopup)
#include "test_popup.moc"
