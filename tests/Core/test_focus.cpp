#include <QTest>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Engine/TerminalEngine.h>

using namespace qamui;

class TestFocus : public QObject {
    Q_OBJECT

private slots:
    void testForceActiveFocus();
    void testFocusChain();
    void testTerminalGeometry();
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

QTEST_MAIN(TestFocus)
#include "test_focus.moc"
