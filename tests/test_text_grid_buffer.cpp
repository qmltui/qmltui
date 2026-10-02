#include <QTest>
#include <QObject>
#include "TextGridBuffer.h"
#include "Text.h"
#include <QString>

using namespace qtui;

class TestTextGridBuffer : public QObject {
    Q_OBJECT

private slots:
    void testResize();
    void testClear();
    void testCellAccess();
    void testDiff();
    void testStyledTextSerialization();
    void testHtmlTagParsing();
    void testTextFormatModes();
};

void TestTextGridBuffer::testResize() {
    TextGridBuffer buffer;
    QCOMPARE(buffer.width(), 0);
    QCOMPARE(buffer.height(), 0);

    buffer.resize(10, 5);
    QCOMPARE(buffer.width(), 10);
    QCOMPARE(buffer.height(), 5);

    Cell c = buffer.cellAt(100, 100);
    QCOMPARE(QString(c.character.data()), QString(""));
}

void TestTextGridBuffer::testClear() {
    TextGridBuffer buffer;
    buffer.resize(5, 5);
    buffer.cellAt(2, 2).character = {'X', '\0', '\0', '\0'};
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString("X"));

    buffer.clear();
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString(" "));
}

void TestTextGridBuffer::testCellAccess() {
    TextGridBuffer buffer;
    buffer.resize(2, 2);
    buffer.cellAt(0, 0).character = {'A', '\0', '\0', '\0'};
    buffer.cellAt(1, 1).character = {'B', '\0', '\0', '\0'};

    QCOMPARE(QString(buffer.cellAt(0, 0).character.data()), QString("A"));
    QCOMPARE(QString(buffer.cellAt(1, 1).character.data()), QString("B"));
    QCOMPARE(QString(buffer.cellAt(0, 1).character.data()), QString(" "));
}

void TestTextGridBuffer::testDiff() {
    TextGridBuffer b1, b2;
    b1.resize(2, 2);
    b2.resize(2, 2);

    b2.cellAt(0, 0).character = {'A', '\0', '\0', '\0'};
    
    std::string diff = b1.diff(b2);
    QVERIFY(diff.find("A") != std::string::npos);
}

void TestTextGridBuffer::testStyledTextSerialization() {
    Text text;
    text.setText("Agent");
    text.setBold(true);

    TextGridBuffer buffer;
    buffer.resize(text.width(), text.height());
    text.render(buffer, 0, 0);

    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QVERIFY(buffer.toAnsiString().find("\033[1m") != std::string::npos);
}

void TestTextGridBuffer::testHtmlTagParsing() {
    Text text;
    text.setText("Normal <b>Bold <font color='red'>RedBold</font> Bold</b> Normal");
    
    // Check that tags are omitted from width
    // Visible text is: "Normal Bold RedBold Bold Normal" -> length: 31
    QCOMPARE(text.width(), 31);
    
    TextGridBuffer buffer;
    buffer.resize(text.width(), text.height());
    text.render(buffer, 0, 0);
    
    // Check characters inside the buffer
    // "Normal " -> 7 chars
    QCOMPARE(QString(buffer.cellAt(6, 0).character.data()), QString(" "));
    QCOMPARE(buffer.cellAt(6, 0).attributes, TextAttribute::None);
    
    // "Bold " -> starts at 7, length 5 (7 to 11)
    QCOMPARE(QString(buffer.cellAt(7, 0).character.data()), QString("B"));
    QCOMPARE(buffer.cellAt(7, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(7, 0).fgColor, 0xFFFFFFFF); // White
    
    // "RedBold" -> starts at 12, length 7
    QCOMPARE(QString(buffer.cellAt(12, 0).character.data()), QString("R"));
    QCOMPARE(buffer.cellAt(12, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(12, 0).fgColor, 0xFFFF0000); // Red
    
    // " Bold" -> starts at 19, length 5
    QCOMPARE(QString(buffer.cellAt(20, 0).character.data()), QString("B"));
    QCOMPARE(buffer.cellAt(20, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(20, 0).fgColor, 0xFFFFFFFF); // Restored to White
    
    // " Normal" -> starts at 24
    QCOMPARE(QString(buffer.cellAt(25, 0).character.data()), QString("N"));
    QCOMPARE(buffer.cellAt(25, 0).attributes, TextAttribute::None); // Restored to None
}

void TestTextGridBuffer::testTextFormatModes() {
    Text text;
    text.setText("<b>Bold</b>");
    
    // AutoText mode (default): parses <b> and hides it
    text.setTextFormat(Text::AutoText);
    QCOMPARE(text.width(), 4);
    
    // PlainText mode: shows <b> verbatim
    text.setTextFormat(Text::PlainText);
    QCOMPARE(text.width(), 11); // "<b>Bold</b>" is 11 chars
    
    // RichText mode: parses HTML but ignores ANSI (represented as \033[1m)
    text.setText("<b>Bold</b>\033[1mAnsi");
    text.setTextFormat(Text::RichText);
    // Visible text should be: "Bold\033[1mAnsi" -> 4 (Bold) + 4 (\033[1) + 4 (mAns) + 1 (i) = 13 (Wait, \033[1m is 4 chars)
    // Actually: "Bold" is 4 chars. "\033[1mAnsi" is 8 chars (\033, [, 1, m, A, n, s, i).
    // So 4 + 8 = 12 characters.
    QCOMPARE(text.width(), 12);
    
    // AnsiText mode: parses ANSI but ignores HTML tags
    text.setTextFormat(Text::AnsiText);
    // HTML tags are not parsed: "<b>Bold</b>" (11 chars) + "Ansi" (4 chars) = 15 chars.
    QCOMPARE(text.width(), 15);
}

QTEST_MAIN(TestTextGridBuffer)
#include "test_text_grid_buffer.moc"
