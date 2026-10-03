#include <QTest>
#include <QObject>
#include "TextInput.h"
#include "KeyEvent.h"

using namespace qtui;

class TestTextInput : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testKeyPress();
    void testBackspace();
    void testCursorMovement();
};

void TestTextInput::testInitialState() {
    TextInput input;
    QCOMPARE(input.text(), QString(""));
    QCOMPARE(input.cursorPosition(), 0);
}

void TestTextInput::testKeyPress() {
    TextInput input;
    KeyEvent ev1(Qt::Key_A, "a", 0);
    input.keyPressEvent(&ev1);

    QCOMPARE(input.text(), QString("a"));
    QCOMPARE(input.cursorPosition(), 1);

    KeyEvent ev2(Qt::Key_B, "b", 0);
    input.keyPressEvent(&ev2);

    QCOMPARE(input.text(), QString("ab"));
    QCOMPARE(input.cursorPosition(), 2);
}

void TestTextInput::testBackspace() {
    TextInput input;
    input.setText("abc");
    input.setCursorPosition(3);

    KeyEvent ev(Qt::Key_Backspace, "", 0);
    input.keyPressEvent(&ev);

    QCOMPARE(input.text(), QString("ab"));
    QCOMPARE(input.cursorPosition(), 2);

    input.setCursorPosition(0);
    input.keyPressEvent(&ev);
    QCOMPARE(input.text(), QString("ab")); // should not change
    QCOMPARE(input.cursorPosition(), 0);
}

void TestTextInput::testCursorMovement() {
    TextInput input;
    input.setText("hello");
    input.setCursorPosition(5);

    KeyEvent evLeft(Qt::Key_Left, "", 0);
    input.keyPressEvent(&evLeft);
    QCOMPARE(input.cursorPosition(), 4);

    KeyEvent evRight(Qt::Key_Right, "", 0);
    input.keyPressEvent(&evRight);
    QCOMPARE(input.cursorPosition(), 5);

    input.keyPressEvent(&evRight); // try to go past end
    QCOMPARE(input.cursorPosition(), 5); // shouldn't go past length
}

QTEST_MAIN(TestTextInput)
#include "test_text_input.moc"
