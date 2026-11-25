#include <QTest>
#include <QSignalSpy>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Items/TextInput.h>
#include <QmlTui/Core/Engine/FocusManager.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>

using namespace qmltui;

class TestFocus : public QObject {
    Q_OBJECT

private slots:
    void testForceActiveFocus();
    void testFocusChain();
    void testTerminalGeometry();
    void testFocusPolicy();
    void testFocusScopeEncapsulation();
    void testFocusScopeNested();
    void testFocusScopeNoDescendantsRecursion();
    void testFocusScopeTabCycleAndVisibility();
};

void TestFocus::testForceActiveFocus() {
    Item parent;
    Item* child1 = new Item(&parent);
    Item* child2 = new Item(&parent);

    QCOMPARE(parent.activeFocus(), false);
    QCOMPARE(child1->activeFocus(), false);
    QCOMPARE(child2->activeFocus(), false);

    child1->setActiveFocus(true);
    QCOMPARE(parent.activeFocus(), false); // doesn't bubble up automatically
    QCOMPARE(child1->activeFocus(), true);
    QCOMPARE(child2->activeFocus(), false);

    child2->setActiveFocus(true);
    child1->setActiveFocus(false); // must be manually cleared for now
    QCOMPARE(parent.activeFocus(), false);
    QCOMPARE(child1->activeFocus(), false); // child1 loses focus
    QCOMPARE(child2->activeFocus(), true);
}

void TestFocus::testFocusChain() {
    Item root;
    Item* level1 = new Item(&root);
    Item* level2 = new Item(level1);

    level2->setActiveFocus(true);

    QCOMPARE(root.activeFocus(), false);
    QCOMPARE(level1->activeFocus(), false);
    QCOMPARE(level2->activeFocus(), true);
}

void TestFocus::testTerminalGeometry() {
    Terminal term;
    auto* engine = term.engine();
    engine->setTestViewportSize(QSize(70, 20));
    QCOMPARE(term.mode(), Terminal::Mode::AltScreen);
    QCOMPARE(term.width(), 70);
    QCOMPARE(term.height(), 20);
    term.setMode(Terminal::Mode::Inline);
    Item child(&term);
    child.setHeight(3);
    QCOMPARE(term.height(), 3);
    child.setHeight(30);
    QCOMPARE(term.height(), 19);
    term.setMode(Terminal::Mode::AltScreen);
    QCOMPARE(term.engine(), engine);
    QCOMPARE(term.height(), 20);
}

void TestFocus::testFocusPolicy() {
    Item item;
    QCOMPARE(item.focusPolicy(), Qt::NoFocus);

    QSignalSpy spy(&item, &Item::focusPolicyChanged);
    item.setFocusPolicy(Qt::StrongFocus);
    QCOMPARE(item.focusPolicy(), Qt::StrongFocus);
    QCOMPARE(spy.count(), 1);

    Terminal term;
    Item child(&term);
    child.setFocusPolicy(Qt::StrongFocus);
    child.setFocus(true);
    QCOMPARE(term.focusedItem(), &child);
    QCOMPARE(child.activeFocus(), true);

    // Setting policy to NoFocus clears focused item
    child.setFocusPolicy(Qt::NoFocus);
    QCOMPARE(term.focusedItem(), nullptr);
    QCOMPARE(child.activeFocus(), false);
}

void TestFocus::testFocusScopeEncapsulation() {
    Terminal term;
    FocusScope scope1(&term);
    FocusScope scope2(&term);

    Item child1A(&scope1);
    child1A.setFocusPolicy(Qt::StrongFocus);
    child1A.setFocus(true); // Internal focus preference

    Item child1B(&scope1);
    child1B.setFocusPolicy(Qt::StrongFocus);

    Item child2A(&scope2);
    child2A.setFocusPolicy(Qt::StrongFocus);
    child2A.setFocus(true); // Internal focus preference

    Item child2B(&scope2);
    child2B.setFocusPolicy(Qt::StrongFocus);

    // Initial state: neither scope is active globally
    QCOMPARE(scope1.activeFocus(), false);
    QCOMPARE(scope2.activeFocus(), false);
    QCOMPARE(child1A.activeFocus(), false);
    QCOMPARE(child2A.activeFocus(), false);
    QCOMPARE(child1A.focus(), true);
    QCOMPARE(child2A.focus(), true);

    // Focus scope1
    scope1.setFocus(true);
    QCOMPARE(scope1.activeFocus(), true);
    QCOMPARE(child1A.activeFocus(), true);
    QCOMPARE(term.focusedItem(), &child1A);
    QCOMPARE(scope2.activeFocus(), false);
    QCOMPARE(child2A.activeFocus(), false);
    QCOMPARE(child2A.focus(), true); // Retains internal focus

    // Switch focus to scope2
    scope2.setFocus(true);
    QCOMPARE(scope1.activeFocus(), false);
    QCOMPARE(child1A.activeFocus(), false);
    QCOMPARE(child1A.focus(), true); // Retains internal focus!
    QCOMPARE(scope2.activeFocus(), true);
    QCOMPARE(child2A.activeFocus(), true);
    QCOMPARE(term.focusedItem(), &child2A);

    // Switch focus within scope2 from child2A to child2B
    child2B.setFocus(true);
    QCOMPARE(child2B.activeFocus(), true);
    QCOMPARE(child2B.focus(), true);
    QCOMPARE(child2A.activeFocus(), false);
    QCOMPARE(child2A.focus(), false);
    QCOMPARE(scope2.activeFocus(), true);
    QCOMPARE(term.focusedItem(), &child2B);

    // Switch focus back to scope1 -> restores child1A!
    scope1.setFocus(true);
    QCOMPARE(scope1.activeFocus(), true);
    QCOMPARE(child1A.activeFocus(), true);
    QCOMPARE(term.focusedItem(), &child1A);
    QCOMPARE(scope2.activeFocus(), false);
    QCOMPARE(child2B.activeFocus(), false);
    QCOMPARE(child2B.focus(), true); // scope2 now remembers child2B
}

void TestFocus::testFocusScopeNested() {
    Terminal term;
    FocusScope outer(&term);
    Item container(&outer); // transparent layout container
    FocusScope inner(&container);
    Item leaf(&inner);
    leaf.setFocusPolicy(Qt::StrongFocus);
    leaf.setFocus(true); // leaf is the focused child of inner

    QCOMPARE(outer.activeFocus(), false);
    QCOMPARE(inner.activeFocus(), false);
    QCOMPARE(leaf.activeFocus(), false);
    QCOMPARE(leaf.focus(), true);

    outer.setFocus(true);
    QCOMPARE(outer.activeFocus(), true);
    QCOMPARE(container.activeFocus(), false);
    QCOMPARE(inner.activeFocus(), true);
    QCOMPARE(leaf.activeFocus(), true);
    QCOMPARE(term.focusedItem(), &leaf);
}

void TestFocus::testFocusScopeNoDescendantsRecursion() {
    Terminal term;
    FocusScope standalone(&term);
    standalone.setFocusPolicy(Qt::StrongFocus);
    standalone.setFocus(true);

    QCOMPARE(term.focusedItem(), &standalone);
    QCOMPARE(standalone.activeFocus(), true);
}

void TestFocus::testFocusScopeTabCycleAndVisibility() {
    Terminal term;
    FocusScope leftScope(&term);
    Item left1(&leftScope);
    left1.setFocusPolicy(Qt::StrongFocus);
    Item left2(&leftScope);
    left2.setFocusPolicy(Qt::StrongFocus);

    FocusScope rightScope(&term);
    TextInput rightInput(&rightScope);
    rightInput.setFocusPolicy(Qt::StrongFocus);
    Item right2(&rightScope);
    right2.setFocusPolicy(Qt::StrongFocus);

    // Hidden parent hides child even though child->visible() is true.
    Item hiddenContainer(&term);
    hiddenContainer.setVisible(false);
    Item hiddenChild(&hiddenContainer);
    hiddenChild.setFocusPolicy(Qt::StrongFocus);

    left1.forceActiveFocus();
    QCOMPARE(term.focusedItem(), &left1);
    QVERIFY(leftScope.activeFocus());
    QVERIFY(!rightScope.activeFocus());

    // Forward Tab cycles through left1 -> left2 -> rightInput -> right2 -> left1.
    FocusManager::cycleFocus(&term, true);
    QCOMPARE(term.focusedItem(), &left2);
    FocusManager::cycleFocus(&term, true);
    QCOMPARE(term.focusedItem(), &rightInput);
    QVERIFY(rightScope.activeFocus());
    QVERIFY(!leftScope.activeFocus());
    FocusManager::cycleFocus(&term, true);
    QCOMPARE(term.focusedItem(), &right2);
    FocusManager::cycleFocus(&term, true);
    QCOMPARE(term.focusedItem(), &left1);

    // Backward Shift+Tab crosses back into rightScope without trapping.
    FocusManager::cycleFocus(&term, false);
    QCOMPARE(term.focusedItem(), &right2);
    QVERIFY(rightScope.activeFocus());

    // Calling forceActiveFocus() on a FocusScope whose child already has focus must not
    // bounce activeFocus and spuriously emit TextInput::editingFinished().
    rightInput.forceActiveFocus();
    QSignalSpy editSpy(&rightInput, &TextInput::editingFinished);
    rightScope.forceActiveFocus();
    QCOMPARE(term.focusedItem(), &rightInput);
    QCOMPARE(editSpy.count(), 0);

    // Effectively hidden items cannot take focus or clear existing focus.
    hiddenChild.forceActiveFocus();
    QCOMPARE(term.focusedItem(), &rightInput);
    term.setFocusedItem(&hiddenChild);
    QCOMPARE(term.focusedItem(), nullptr);
}

QTEST_MAIN(TestFocus)
#include "test_focus.moc"
