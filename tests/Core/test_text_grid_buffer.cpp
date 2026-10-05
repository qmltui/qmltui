#include <QTest>
#include <QObject>
#include "TextGridBuffer.h"
#include "Text.h"
#include "Color.h"
#include <QString>

#include <limits>

using namespace qtui;

class TestTextGridBuffer : public QObject {
    Q_OBJECT

private slots:
    void testResize();
    void testClear();
    void testCellAccess();
    void testColorValue();
    void testDiff();
    void testDiffBatchesContiguousCells();
    void testDiffReusesStyleAcrossRuns();
    void testDiffEmitsStyleTransitions();
    void testDiffHandlesWideCharacterTransitions();
    void testRelativeDiffBatchesContiguousCells();
    void testStyledTextSerialization();
    void testExtendedAttributeSerialization();
    void testHtmlTagParsing();
    void testTextFormatModes();
    void testDrawingHelpers();
    void testInvalidResize();
    void testControlCharactersAreVisible();
};

void TestTextGridBuffer::testResize() {
    TextGridBuffer buffer;
    QCOMPARE(buffer.width(), 0);
    QCOMPARE(buffer.height(), 0);

    buffer.resize(10, 5);
    QCOMPARE(buffer.width(), 10);
    QCOMPARE(buffer.height(), 5);

    QVERIFY(buffer.contains(0, 0));
    QVERIFY(!buffer.contains(100, 100));
    QCOMPARE(buffer.tryCellAt(100, 100), nullptr);
    const TextGridBuffer &constBuffer = buffer;
    QCOMPARE(constBuffer.tryCellAt(100, 100), nullptr);
}

void TestTextGridBuffer::testClear() {
    TextGridBuffer buffer;
    buffer.resize(5, 5);
    buffer.cellAt(2, 2).character = "X";
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString("X"));

    buffer.clear();
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString(" "));
}

void TestTextGridBuffer::testCellAccess() {
    TextGridBuffer buffer;
    buffer.resize(2, 2);
    buffer.cellAt(0, 0).character = "A";
    buffer.cellAt(1, 1).character = "B";

    QCOMPARE(QString(buffer.cellAt(0, 0).character.data()), QString("A"));
    QCOMPARE(QString(buffer.cellAt(1, 1).character.data()), QString("B"));
    QCOMPARE(QString(buffer.cellAt(0, 1).character.data()), QString(" "));

    QVERIFY(buffer.contains(0, 0));
    QVERIFY(buffer.contains(1, 1));
    QVERIFY(!buffer.contains(-1, 0));
    QVERIFY(!buffer.contains(0, -1));
    QVERIFY(!buffer.contains(2, 0));
    QVERIFY(!buffer.contains(0, 2));
    QVERIFY(buffer.tryCellAt(1, 0));
    QCOMPARE(buffer.tryCellAt(-1, 0), nullptr);
    QCOMPARE(buffer.tryCellAt(2, 1), nullptr);

    const TextGridBuffer &constBuffer = buffer;
    QVERIFY(constBuffer.tryCellAt(0, 1));
    QCOMPARE(constBuffer.tryCellAt(0, 2), nullptr);
}

void TestTextGridBuffer::testColorValue() {
    const Color red(" red ");
    QVERIFY(red.isValid());
    QCOMPARE(red.argb32(), 0xffff0000U);
    QCOMPARE(red.toString(), QStringLiteral("#ff0000"));

    QCOMPARE(Color("#0f8").argb32(), 0xff00ff88U);
    QCOMPARE(Color("#80402010").argb32(), 0x80402010U);
    QCOMPARE(Color("DarkSlateGrey").argb32(), 0xff2f4f4fU);
    QCOMPARE(Color("aliceblue").argb32(), 0xfff0f8ffU);
    QCOMPARE(Color::fromRgba(0.2, 0.4, 0.6, 0.5).argb32(), 0x80336699U);
    QCOMPARE(Color::fromRgba(-1.0, 2.0, 0.5, 2.0).argb32(), 0xff00ff80U);
    QCOMPARE(Color::fromRgba(1.0, 0.0, 0.0).argb32(), 0xffff0000U);
    QVERIFY(!Color::fromRgba(std::numeric_limits<qreal>::quiet_NaN(),
                             0.0, 0.0, 1.0).isValid());
    QVERIFY(!Color().isValid());
    QVERIFY(!Color("not-a-color").isValid());
    QVERIFY(!Color("#xyz").isValid());

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(2, 1));
    buffer.writeText(0, 0, QStringLiteral("X"), Color("tomato"),
                     Color("navy"));
    QCOMPARE(buffer.cellAt(0, 0).foreground(), Color("tomato"));
    QCOMPARE(buffer.cellAt(0, 0).background(), Color("navy"));

    Cell &cell = buffer.cellAt(1, 0);
    cell.setForeground(Color("gold"));
    cell.setBackground(Color("transparent"));
    QCOMPARE(cell.fgColor, 0xffffd700U);
    QCOMPARE(cell.bgColor, 0x00000000U);
}

void TestTextGridBuffer::testDiff() {
    TextGridBuffer b1, b2;
    b1.resize(2, 2);
    b2.resize(2, 2);

    b2.cellAt(0, 0).character = "A";
    
    std::string diff = b1.diff(b2);
    QVERIFY(diff.find("A") != std::string::npos);
    QVERIFY(b2.diff(b2).empty());
}

static int occurrenceCount(const std::string& text, const std::string& needle) {
    int count = 0;
    size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

void TestTextGridBuffer::testDiffBatchesContiguousCells() {
    TextGridBuffer before, after;
    before.resize(80, 1);
    after.resize(80, 1);
    after.fillRect(0, 0, 80, 1, "X");

    const std::string diff = before.diff(after);
    QCOMPARE(occurrenceCount(diff, "\033[1;1H"), 1);
    QCOMPARE(occurrenceCount(diff, "\033[1;2H"), 0);
    QCOMPARE(occurrenceCount(diff, "\033[0m"), 1);
    QVERIFY(diff.size() < 200);
    QVERIFY(diff.find(std::string(80, 'X')) != std::string::npos);
}

void TestTextGridBuffer::testDiffReusesStyleAcrossRuns() {
    TextGridBuffer before, after;
    before.resize(5, 1);
    after.resize(5, 1);
    after.cellAt(0, 0).character = "A";
    after.cellAt(2, 0).character = "B";

    const std::string diff = before.diff(after);
    QCOMPARE(occurrenceCount(diff, "\033[0m"), 1);
    QCOMPARE(occurrenceCount(diff, "38;2;255;255;255"), 1);
    QCOMPARE(occurrenceCount(diff, "H"), 2);
}

void TestTextGridBuffer::testDiffEmitsStyleTransitions() {
    TextGridBuffer before, after;
    before.resize(2, 1);
    after.resize(2, 1);
    after.cellAt(0, 0).character = "A";
    after.cellAt(0, 0).fgColor = 0xFFFF0000;
    after.cellAt(0, 0).bgColor = 0xFF0000FF;
    after.cellAt(0, 0).attributes = TextAttribute::Bold;
    after.cellAt(1, 0).character = "B";

    const std::string diff = before.diff(after);
    QVERIFY(diff.find("38;2;255;0;0") != std::string::npos);
    QVERIFY(diff.find("48;2;0;0;255") != std::string::npos);
    QVERIFY(diff.find("38;2;255;255;255;49;22m") != std::string::npos);
    QVERIFY(diff.ends_with("B"));
}

void TestTextGridBuffer::testDiffHandlesWideCharacterTransitions() {
    TextGridBuffer before, wide, narrow;
    before.resize(3, 1);
    wide.resize(3, 1);
    narrow.resize(3, 1);
    wide.writeText(0, 0, QString::fromUtf8("中A"));
    narrow.writeText(0, 0, QStringLiteral("A"));

    const std::string toWide = before.diff(wide);
    QCOMPARE(occurrenceCount(toWide, "H"), 1);
    QVERIFY(toWide.find(QString::fromUtf8("中A").toStdString()) != std::string::npos);

    const std::string toNarrow = wide.diff(narrow);
    QCOMPARE(occurrenceCount(toNarrow, "H"), 1);
    QVERIFY(toNarrow.ends_with("A  "));
}

void TestTextGridBuffer::testRelativeDiffBatchesContiguousCells() {
    TextGridBuffer before, after;
    before.resize(8, 2);
    after.resize(8, 2);
    after.fillRect(2, 1, 4, 1, "R");

    const std::string diff = before.diffRelative(after);
    QCOMPARE(occurrenceCount(diff, "\033[1B"), 1);
    QCOMPARE(occurrenceCount(diff, "\033[2C"), 1);
    QVERIFY(diff.find("RRRR") != std::string::npos);
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

void TestTextGridBuffer::testExtendedAttributeSerialization() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(1, 1));
    Cell& cell = buffer.cellAt(0, 0);
    cell.character = "X";
    cell.attributes = TextAttribute::Blink | TextAttribute::Conceal
        | TextAttribute::Strikethrough;

    const std::string ansi = buffer.toAnsiString();
    QVERIFY(ansi.find("\033[5m") != std::string::npos);
    QVERIFY(ansi.find("\033[8m") != std::string::npos);
    QVERIFY(ansi.find("\033[9m") != std::string::npos);
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
    QCOMPARE(text.width(), 11); // ESC is a zero-width control character.
    
    // AnsiText mode: parses ANSI but ignores HTML tags
    text.setTextFormat(Text::AnsiText);
    // HTML tags are not parsed: "<b>Bold</b>" (11 chars) + "Ansi" (4 chars) = 15 chars.
    QCOMPARE(text.width(), 15);
}

void TestTextGridBuffer::testDrawingHelpers() {
    TextGridBuffer buffer;
    buffer.resize(10, 5);

    // Test writeText
    buffer.writeText(1, 1, "Hello", 0xFF0000FF, 0x00000000, TextAttribute::Bold);
    QCOMPARE(QString(buffer.cellAt(1, 1).character.data()), QString("H"));
    QCOMPARE(QString(buffer.cellAt(5, 1).character.data()), QString("o"));
    QCOMPARE(buffer.cellAt(1, 1).fgColor, 0xFF0000FF);
    QCOMPARE(buffer.cellAt(1, 1).attributes, TextAttribute::Bold);

    // Test fillRect
    buffer.fillRect(0, 0, 3, 2, "#");
    QCOMPARE(QString(buffer.cellAt(0, 0).character.data()), QString("#"));
    QCOMPARE(QString(buffer.cellAt(2, 1).character.data()), QString("#"));

    // Test drawLine
    buffer.drawLine(0, 3, 4, 3, "-");
    QCOMPARE(QString(buffer.cellAt(0, 3).character.data()), QString("-"));
    QCOMPARE(QString(buffer.cellAt(4, 3).character.data()), QString("-"));

    // Test drawBox
    TextGridBuffer boxBuf;
    boxBuf.resize(4, 4);
    boxBuf.drawBox(0, 0, 4, 4);
    QCOMPARE(boxBuf.cellAt(0, 0).character[0], '\xE2'); // ┌ (UTF-8 3-byte char)
    QCOMPARE(boxBuf.cellAt(3, 0).character[0], '\xE2'); // ┐
    QCOMPARE(boxBuf.cellAt(0, 3).character[0], '\xE2'); // └
    QCOMPARE(boxBuf.cellAt(3, 3).character[0], '\xE2'); // ┘
}

void TestTextGridBuffer::testInvalidResize() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 4));
    QVERIFY(!buffer.resize(-1, 4));
    QVERIFY(!buffer.resize(100000, 100000));
    QCOMPARE(buffer.width(), 4);
    QCOMPARE(buffer.height(), 4);
}

void TestTextGridBuffer::testControlCharactersAreVisible() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    buffer.writeText(0, 0, QString(QChar(0x1b)));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 0).character), QString(QChar(0x241b)));
}

QTEST_MAIN(TestTextGridBuffer)
#include "test_text_grid_buffer.moc"
