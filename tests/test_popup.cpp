#include <QTest>
#include <QObject>
#include <QSocketNotifier>
#include <QSignalSpy>
#include <sstream>
#include <chrono>
#include <vector>
#include <iostream>

#include "Terminal.h"
#include "Popup.h"
#include "MouseArea.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

#define private public
#include "TerminalEngine.h"
#undef private

using namespace qtui;

class TestPopup : public QObject {
    Q_OBJECT

private slots:
    void testPopupRegistration();
    void testModalInputBlocking();
    void testClosePolicies();
    void testBaseVisibilityDispatchAndParentNotification();
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
    terminal.setWidth(80);
    terminal.setHeight(24);

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

QTEST_MAIN(TestPopup)
#include "test_popup.moc"
