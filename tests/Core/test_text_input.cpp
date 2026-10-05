#include <QTest>
#include <QObject>
#include <QRegularExpression>
#include <QQmlComponent>
#include <QQmlEngine>
#include <memory>
#include "TextInput.h"
#include "KeyEvent.h"
#include "TextGridBuffer.h"
#include "../TestPaint.h"

using namespace qtui;
using qtui::testing::paintItem;

class TestTextInput : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testKeyPress();
    void testBackspace();
    void testCursorMovement();
    void testSingleLineSanitizationAndLimit();
    void testGraphemeCursorInvariant();
    void testSelectionUndoRedo();
    void testNavigationSelection();
    void testWordEditing();
    void testHorizontalViewport();
    void testFieldPropertiesAndPasswordRendering();
    void testTerminalCursorCustomization();
    void testQmlSurface();
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

void TestTextInput::testSingleLineSanitizationAndLimit() {
    TextInput input;
    input.setText("a\r\nb\tc");
    QCOMPARE(input.text(), QString("a b c"));

    input.setText("");
    input.setMaximumLength(3);
    KeyEvent paste(0, "ab\ncd", 0);
    input.keyPressEvent(&paste);

    QVERIFY(paste.isAccepted());
    QCOMPARE(input.text(), QString("ab "));
    QCOMPARE(input.cursorPosition(), 3);

    KeyEvent extra(Qt::Key_X, "x", 0);
    input.keyPressEvent(&extra);
    QCOMPARE(input.text(), QString("ab "));
}

void TestTextInput::testGraphemeCursorInvariant() {
    TextInput input;
    input.setText(QString::fromUtf8("😀x"));

    input.setCursorPosition(1); // Inside the emoji's UTF-16 surrogate pair.
    QCOMPARE(input.cursorPosition(), 0);

    KeyEvent deleteEvent(Qt::Key_Delete, "", 0);
    input.keyPressEvent(&deleteEvent);
    QCOMPARE(input.text(), QString("x"));
    QCOMPARE(input.cursorPosition(), 0);

    input.undo();
    QCOMPARE(input.text(), QString::fromUtf8("😀x"));
}

void TestTextInput::testSelectionUndoRedo() {
    TextInput input;
    input.setText("abc");
    input.setCursorPosition(3);

    KeyEvent selectLeft(Qt::Key_Left, "", Qt::ShiftModifier);
    input.keyPressEvent(&selectLeft);
    QCOMPARE(input.selectionStart(), 2);
    QCOMPARE(input.selectionEnd(), 3);
    QCOMPARE(input.selectedText(), QString("c"));

    KeyEvent replace(Qt::Key_X, "X", 0);
    input.keyPressEvent(&replace);
    QCOMPARE(input.text(), QString("abX"));
    QCOMPARE(input.selectionStart(), 3);
    QCOMPARE(input.selectionEnd(), 3);
    QVERIFY(input.canUndo());

    input.undo();
    QCOMPARE(input.text(), QString("abc"));
    QCOMPARE(input.selectedText(), QString("c"));
    QVERIFY(input.canRedo());

    input.redo();
    QCOMPARE(input.text(), QString("abX"));
}

void TestTextInput::testNavigationSelection() {
    TextInput input;
    input.setText("one two");

    input.setCursorPosition(4);
    KeyEvent shiftHome(Qt::Key_Home, "", Qt::ShiftModifier);
    input.keyPressEvent(&shiftHome);
    QCOMPARE(input.selectionStart(), 0);
    QCOMPARE(input.selectionEnd(), 4);
    QCOMPARE(input.selectedText(), QString("one "));

    input.setCursorPosition(3);
    KeyEvent shiftEnd(Qt::Key_End, "", Qt::ShiftModifier);
    input.keyPressEvent(&shiftEnd);
    QCOMPARE(input.selectionStart(), 3);
    QCOMPARE(input.selectionEnd(), 7);
    QCOMPARE(input.selectedText(), QString(" two"));

    input.setCursorPosition(7);
    KeyEvent controlShiftLeft(
        Qt::Key_Left, "", Qt::ControlModifier | Qt::ShiftModifier);
    input.keyPressEvent(&controlShiftLeft);
    QCOMPARE(input.selectionStart(), 4);
    QCOMPARE(input.selectionEnd(), 7);
    QCOMPARE(input.selectedText(), QString("two"));

    KeyEvent collapseLeft(Qt::Key_Left, "", 0);
    input.keyPressEvent(&collapseLeft);
    QCOMPARE(input.cursorPosition(), 4);
    QCOMPARE(input.selectionStart(), input.selectionEnd());

    input.setCursorPosition(0);
    KeyEvent controlShiftRight(
        Qt::Key_Right, "", Qt::ControlModifier | Qt::ShiftModifier);
    input.keyPressEvent(&controlShiftRight);
    QCOMPARE(input.cursorPosition(), 3);
    QCOMPARE(input.selectedText(), QString("one"));

    input.keyPressEvent(&controlShiftRight);
    QCOMPARE(input.cursorPosition(), 7);
    QCOMPARE(input.selectedText(), QString("one two"));
}

void TestTextInput::testWordEditing() {
    TextInput input;
    input.setText("one two");
    input.setCursorPosition(input.text().size());

    KeyEvent wordLeft(Qt::Key_Left, "", Qt::ControlModifier);
    input.keyPressEvent(&wordLeft);
    QCOMPARE(input.cursorPosition(), 4);

    KeyEvent wordBackspace(
        Qt::Key_Backspace, "", Qt::ControlModifier);
    input.keyPressEvent(&wordBackspace);
    QCOMPARE(input.text(), QString("two"));
    QCOMPARE(input.cursorPosition(), 0);
}

void TestTextInput::testHorizontalViewport() {
    TextInput input;
    input.setWidth(4);
    input.setHeight(1);
    input.setText("abcdef");
    input.setCursorPosition(6);
    input.setActiveFocus(true);

    QCOMPARE(input.contentX(), 3);

    TextGridBuffer buffer;
    buffer.resize(4, 1);
    paintItem(input, buffer, 0, 0, ClipRect{0, 0, 4, 1});
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("d"));
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray("e"));
    QCOMPARE(buffer.cellAt(2, 0).character().toByteArray(), QByteArray("f"));
    QVERIFY((buffer.cellAt(3, 0).attributes & TextAttribute::Reversed)
            != TextAttribute::None);
}

void TestTextInput::testFieldPropertiesAndPasswordRendering() {
    TextInput input;
    input.setValidator(QRegularExpression("[0-9]+"));
    QVERIFY(!input.acceptableInput());
    input.setText("12");
    QVERIFY(input.acceptableInput());
    input.setText("12a");
    QVERIFY(!input.acceptableInput());

    input.setWidth(4);
    input.setHeight(1);
    input.setText("secret");
    input.setEchoMode(TextInput::Password);
    input.setCursorPosition(0);

    TextGridBuffer buffer;
    buffer.resize(4, 1);
    paintItem(input, buffer, 0, 0, ClipRect{0, 0, 4, 1});
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("*"));
    QVERIFY(buffer.cellAt(0, 0).character().toByteArray() != QByteArray("s"));

    input.setReadOnly(true);
    KeyEvent typed(Qt::Key_X, "x", 0);
    input.keyPressEvent(&typed);
    QCOMPARE(input.text(), QString("secret"));
}

void TestTextInput::testTerminalCursorCustomization() {
    TextInput input;
    input.setText("hello");
    input.setActiveFocus(true);

    QCOMPARE(input.cursorShape(), TextInput::Block);
    QCOMPARE(input.cursorBlinking(), false);
    QCOMPARE(input.cursorVisible(), true);

    input.setCursorShape(TextInput::Beam);
    QCOMPARE(input.cursorShape(), TextInput::Beam);

    input.setCursorColor("cyan");
    QCOMPARE(input.cursorColor(), QString("cyan"));

    input.setCursorBlinking(true);
    QCOMPARE(input.cursorBlinking(), true);

    input.setCursorBlinkRate(250);
    QCOMPARE(input.cursorBlinkRate(), 250);

    input.setCursorDelegate("▋");
    QCOMPARE(input.cursorDelegate(), QString("▋"));

    // Test cursor at position 0 (over character 'h') - character 'h' must be preserved!
    input.setCursorPosition(0);
    TextGridBuffer buffer;
    buffer.resize(10, 1);
    paintItem(input, buffer, 0, 0, ClipRect{0, 0, 10, 1});

    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("h"));
    QCOMPARE(buffer.cellAt(0, 0).bgColor, static_cast<uint32_t>(0xFF00FFFF)); // cyan bg highlight

    // Test cursor at position 5 (end of text) - delegate glyph "▋" rendered at empty position
    input.setCursorPosition(5);
    buffer = TextGridBuffer();
    buffer.resize(10, 1);
    paintItem(input, buffer, 0, 0, ClipRect{0, 0, 10, 1});

    QCOMPARE(buffer.cellAt(5, 0).character().toByteArray(), QByteArray("▋"));
    QCOMPARE(buffer.cellAt(5, 0).fgColor, static_cast<uint32_t>(0xFF00FFFF));

    input.setSelectionColor("blue");
    input.setSelectedTextColor("yellow");
    QCOMPARE(input.selectionColor(), QString("blue"));
    QCOMPARE(input.selectedTextColor(), QString("yellow"));

    input.select(0, 3);
    paintItem(input, buffer, 0, 0, ClipRect{0, 0, 10, 1});

    QCOMPARE(buffer.cellAt(1, 0).bgColor, static_cast<uint32_t>(0xFF0000FF)); // blue ARGB
    QCOMPARE(buffer.cellAt(1, 0).fgColor, static_cast<uint32_t>(0xFFFFFF00)); // yellow ARGB

    // Test clear(), overwriteMode(), placeholderColor(), autoScroll()
    input.setPlaceholderText("Enter name...");
    input.setPlaceholderColor("gray");
    QCOMPARE(input.placeholderColor(), QString("gray"));

    input.clear();
    QCOMPARE(input.text(), QString(""));
    QCOMPARE(input.cursorPosition(), 0);

    input.setOverwriteMode(true);
    QCOMPARE(input.overwriteMode(), true);
    input.setText("abcd");
    input.setCursorPosition(1);

    KeyEvent overwriteEv(Qt::Key_X, "x", 0);
    input.keyPressEvent(&overwriteEv);
    QCOMPARE(input.text(), QString("axcd"));
    QCOMPARE(input.cursorPosition(), 2);
}

void TestTextInput::testQmlSurface() {
    qmlRegisterType<TextInput>("QtuiTextInputTest", 1, 0, "TextInput");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QtuiTextInputTest 1.0

        TextInput {
            text: "123"
            maximumLength: 2
            placeholderText: "digits"
            echoMode: TextInput.Password
            validator: /[0-9]+/
        }
    )", QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    TextInput *input = qobject_cast<TextInput *>(object.get());
    QVERIFY(input);
    QCOMPARE(input->text(), QString("12"));
    QCOMPARE(input->placeholderText(), QString("digits"));
    QCOMPARE(input->echoMode(), TextInput::Password);
    QVERIFY(input->acceptableInput());
}

QTEST_MAIN(TestTextInput)
#include "test_text_input.moc"
