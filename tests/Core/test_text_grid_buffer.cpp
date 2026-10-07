#include <QTest>
#include <QObject>
#include "TextGridBuffer.h"
#include "Text.h"
#include "Color.h"
#include "../TestPaint.h"
#include <QString>
#include "ColorMapper.h"

#include <limits>

using namespace qtui;
using qtui::testing::paintItem;

class TestTextGridBuffer : public QObject {
    Q_OBJECT

private slots:
    void testResize();
    void testClear();
    void testCellAccess();
    void testColorValue();
    void testDefaultForeground();
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
    void testWriteTextAppendsZeroWidthCluster();
    void testWriteTextLongCluster();
    void testCopiedBufferComparesEqual();
    void testToAnsiStringExactRow();
    void testFillRectPreservesAttributes();
    void testSetGraphemeWideAndContinuation();
    void testSetGraphemeZeroWidthAppendsToBase();
    void testSetGraphemeClipsAtRightEdge();
    void testSetGraphemeTransparentBgPreservesUnderlying();
    void testSetGraphemeOutsideBufferAdvances();
    void testDiffUsesEraseToEndOfLine();
    void testDiffEraseRespectsMinimum();
    void testDiffEraseNotUsedWhenTailStyled();
    void testDiffAppendsIntoCallerString();
    void testDiffLargeCoordinates();
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
    buffer.cellAt(2, 2).setCharacter("X");
    QCOMPARE(QString::fromUtf8(buffer.cellAt(2, 2).character().toByteArray()), QString("X"));

    buffer.clear();
    QCOMPARE(QString::fromUtf8(buffer.cellAt(2, 2).character().toByteArray()), QString(" "));
}

void TestTextGridBuffer::testCellAccess() {
    TextGridBuffer buffer;
    buffer.resize(2, 2);
    buffer.cellAt(0, 0).setCharacter("A");
    buffer.cellAt(1, 1).setCharacter("B");

    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 0).character().toByteArray()), QString("A"));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(1, 1).character().toByteArray()), QString("B"));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 1).character().toByteArray()), QString(" "));

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
    QVERIFY(red.isAnsiIndex());                       // "red" is palette entry 9, not RGB
    QCOMPARE(red.toRgb().argb32(), 0xffff0000U);
    QCOMPARE(red.toString(), QStringLiteral("red"));
    QCOMPARE(Color("#ff0000").toString(), QStringLiteral("#ff0000"));

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

    b2.cellAt(0, 0).setCharacter("A");
    
    std::string diff = b1.diff(b2);
    QVERIFY(diff.find("A") != std::string::npos);
    QVERIFY(b2.diff(b2).empty());
}

void TestTextGridBuffer::testDefaultForeground() {
    Cell cell;
    cell.setCharacter("X");
    QCOMPARE(cell.fgColor, 0u);
    QCOMPARE(cell.bgColor, 0u);

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(10, 8));
    buffer.cellAt(0, 0).setCharacter("X");
    buffer.writeText(0, 1, QStringLiteral("Text"));
    buffer.setGrapheme(0, 2, QByteArrayView("A"), 1);
    const QString wide = QString::fromUtf8("中");
    buffer.setGrapheme(0, 3, QStringView(wide), 2);
    buffer.drawLine(0, 4, 9, 4);
    buffer.drawBox(0, 5, 4, 3);
    buffer.fillRect(5, 5, 4, 3, "#");
    for (int y = 0; y < buffer.height(); ++y)
        for (int x = 0; x < buffer.width(); ++x) {
            QCOMPARE(buffer.cellAt(x, y).fgColor, 0u);
            QCOMPARE(buffer.cellAt(x, y).bgColor, 0u);
        }
    QVERIFY(buffer.cellAt(1, 3).continuation);

    buffer.writeText(0, 0, QStringLiteral("White"), 0xFFFFFFFF);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xFFFFFFFFu);
    buffer.clear();
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0u);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
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
    after.cellAt(0, 0).setCharacter("A");
    after.cellAt(2, 0).setCharacter("B");

    const std::string diff = before.diff(after);
    QCOMPARE(occurrenceCount(diff, "\033[0m"), 1);
    QCOMPARE(occurrenceCount(diff, "38;"), 0);
    QCOMPARE(occurrenceCount(diff, "H"), 2);
}

void TestTextGridBuffer::testDiffEmitsStyleTransitions() {
    TextGridBuffer before, after;
    before.resize(2, 1);
    after.resize(2, 1);
    after.cellAt(0, 0).setCharacter("A");
    after.cellAt(0, 0).fgColor = 0xFFFF0000;
    after.cellAt(0, 0).bgColor = 0xFF0000FF;
    after.cellAt(0, 0).attributes = TextAttribute::Bold;
    after.cellAt(1, 0).setCharacter("B");

    const std::string diff = before.diff(after);
    QVERIFY(diff.find("38;2;255;0;0") != std::string::npos);
    QVERIFY(diff.find("48;2;0;0;255") != std::string::npos);
    QVERIFY(diff.find("39;49;22m") != std::string::npos);
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
    paintItem(text, buffer, 0, 0);

    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QVERIFY(buffer.toAnsiString().find("\033[1m") != std::string::npos);
}

void TestTextGridBuffer::testExtendedAttributeSerialization() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(1, 1));
    Cell& cell = buffer.cellAt(0, 0);
    cell.setCharacter("X");
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
    paintItem(text, buffer, 0, 0);
    
    // Check characters inside the buffer
    // "Normal " -> 7 chars
    QCOMPARE(QString::fromUtf8(buffer.cellAt(6, 0).character().toByteArray()), QString(" "));
    QCOMPARE(buffer.cellAt(6, 0).attributes, TextAttribute::None);
    
    // "Bold " -> starts at 7, length 5 (7 to 11)
    QCOMPARE(QString::fromUtf8(buffer.cellAt(7, 0).character().toByteArray()), QString("B"));
    QCOMPARE(buffer.cellAt(7, 0).attributes, TextAttribute::Bold);
    // Text's default colour is the terminal's own foreground (0 -> SGR 39), not
    // a literal RGB white that would vanish on a light theme.
    QCOMPARE(buffer.cellAt(7, 0).fgColor, 0u);
    
    // "RedBold" -> starts at 12, length 7
    QCOMPARE(QString::fromUtf8(buffer.cellAt(12, 0).character().toByteArray()), QString("R"));
    QCOMPARE(buffer.cellAt(12, 0).attributes, TextAttribute::Bold);
    QCOMPARE(ColorMapper::toArgb(buffer.cellAt(12, 0).fgColor), 0xFFFF0000u); // Red
    
    // " Bold" -> starts at 19, length 5
    QCOMPARE(QString::fromUtf8(buffer.cellAt(20, 0).character().toByteArray()), QString("B"));
    QCOMPARE(buffer.cellAt(20, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(20, 0).fgColor, 0u); // restored to the terminal default
    
    // " Normal" -> starts at 24
    QCOMPARE(QString::fromUtf8(buffer.cellAt(25, 0).character().toByteArray()), QString("N"));
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
    QCOMPARE(QString::fromUtf8(buffer.cellAt(1, 1).character().toByteArray()), QString("H"));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(5, 1).character().toByteArray()), QString("o"));
    QCOMPARE(buffer.cellAt(1, 1).fgColor, 0xFF0000FF);
    QCOMPARE(buffer.cellAt(1, 1).attributes, TextAttribute::Bold);

    // Test fillRect
    buffer.fillRect(0, 0, 3, 2, "#");
    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 0).character().toByteArray()), QString("#"));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(2, 1).character().toByteArray()), QString("#"));

    // Test drawLine
    buffer.drawLine(0, 3, 4, 3, "-");
    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 3).character().toByteArray()), QString("-"));
    QCOMPARE(QString::fromUtf8(buffer.cellAt(4, 3).character().toByteArray()), QString("-"));

    // Test drawBox
    TextGridBuffer boxBuf;
    boxBuf.resize(4, 4);
    boxBuf.drawBox(0, 0, 4, 4);
    QCOMPARE(boxBuf.cellAt(0, 0).character()[0], '\xE2'); // ┌ (UTF-8 3-byte char)
    QCOMPARE(boxBuf.cellAt(3, 0).character()[0], '\xE2'); // ┐
    QCOMPARE(boxBuf.cellAt(0, 3).character()[0], '\xE2'); // └
    QCOMPARE(boxBuf.cellAt(3, 3).character()[0], '\xE2'); // ┘
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
    QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 0).character().toByteArray()), QString(QChar(0x241b)));
}

void TestTextGridBuffer::testWriteTextAppendsZeroWidthCluster() {
    // A zero-width cluster written after a base character joins that cell
    // rather than occupying its own column.
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    buffer.writeText(0, 0, "a");
    buffer.writeText(1, 0, QString(QChar(0x0301)));
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("a\xCC\x81"));
    QCOMPARE(buffer.cellAt(0, 0).displayWidth, uint8_t(1));
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray(" "));
}

void TestTextGridBuffer::testWriteTextLongCluster() {
    // A ZWJ emoji sequence (18 UTF-8 bytes) occupies one wide cell plus one
    // continuation cell and keeps every byte.
    const QString family = QString::fromUtf8("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7");
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    buffer.writeText(1, 0, family, 0xFF00FF00, 0xFF000080);
    QCOMPARE(buffer.cellAt(1, 0).character().size(), 18);
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), family.toUtf8());
    QCOMPARE(buffer.cellAt(1, 0).displayWidth, uint8_t(2));
    QVERIFY(!buffer.cellAt(1, 0).continuation);
    QVERIFY(buffer.cellAt(2, 0).continuation);
    QVERIFY(buffer.cellAt(2, 0).character().isEmpty());
    QCOMPARE(buffer.cellAt(2, 0).displayWidth, uint8_t(0));
    QCOMPARE(buffer.cellAt(2, 0).bgColor, 0xFF000080u);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.cellAt(3, 0).character().toByteArray(), QByteArray(" "));
}

void TestTextGridBuffer::testCopiedBufferComparesEqual() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(6, 2));
    buffer.writeText(0, 0, QString::fromUtf8("A\xE4\xB8\xAD\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"), 0xFFFF0000, 0xFF0000FF, TextAttribute::Bold);
    buffer.fillRect(0, 1, 6, 1, "#", 0xFF00FF00);

    const TextGridBuffer copy = buffer;
    QVERIFY(buffer.diff(copy).empty());
    QVERIFY(copy.diff(buffer).empty());
    for (int x = 0; x < 6; ++x)
        for (int y = 0; y < 2; ++y)
            QVERIFY(copy.cellAt(x, y) == buffer.cellAt(x, y));

    TextGridBuffer swapped;
    QVERIFY(swapped.resize(6, 2));
    swapped.swap(buffer);
    QVERIFY(swapped.diff(copy).empty());
}

void TestTextGridBuffer::testToAnsiStringExactRow() {
    // Exact bytes for one styled row, so encoder rewrites are provably
    // byte-identical for the full-frame serialization.
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(3, 1));
    buffer.writeText(0, 0, "AB", 0xFFFF0000, 0xFF0000FF, TextAttribute::Bold);
    const std::string expected =
        "\033[0m\033[38;2;255;0;0m\033[48;2;0;0;255m\033[1mAB"
        "\033[0m \033[0m";
    QCOMPARE(QByteArray::fromStdString(buffer.toAnsiString()), QByteArray::fromStdString(expected));
}

void TestTextGridBuffer::testFillRectPreservesAttributes() {
    // fillRect, drawLine and drawBox replace glyph and colours but leave
    // attributes untouched; Rectangle's background fill leaves fg untouched.
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(3, 1));
    buffer.writeText(0, 0, "xyz", 0xFF123456, 0xFF654321, TextAttribute::Underline);
    buffer.fillRect(0, 0, 2, 1, " ", 0xFFFFFFFF, 0xFF000000);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xFFFFFFFFu);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xFF000000u);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Underline);
    QCOMPARE(buffer.cellAt(2, 0).character().toByteArray(), QByteArray("z"));
    QCOMPARE(buffer.cellAt(2, 0).fgColor, 0xFF123456u);
}

void TestTextGridBuffer::testSetGraphemeWideAndContinuation() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    QCOMPARE(buffer.setGrapheme(1, 0, "\xE4\xB8\xAD", 2, 0xFF00FF00, 0xFF000080, TextAttribute::Bold), 2);
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray("\xE4\xB8\xAD"));
    QCOMPARE(buffer.cellAt(1, 0).displayWidth, uint8_t(2));
    QVERIFY(!buffer.cellAt(1, 0).continuation);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Bold);
    QVERIFY(buffer.cellAt(2, 0).continuation);
    QVERIFY(buffer.cellAt(2, 0).character().isEmpty());
    QCOMPARE(buffer.cellAt(2, 0).displayWidth, uint8_t(0));
    QCOMPARE(buffer.cellAt(2, 0).fgColor, 0xFF00FF00u);
    QCOMPARE(buffer.cellAt(2, 0).bgColor, 0xFF000080u);
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.cellAt(3, 0).character().toByteArray(), QByteArray(" "));
}

void TestTextGridBuffer::testSetGraphemeZeroWidthAppendsToBase() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    QCOMPARE(buffer.setGrapheme(0, 0, "a", 1), 1);
    QCOMPARE(buffer.setGrapheme(1, 0, "\xCC\x81", 0), 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("a\xCC\x81"));
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray(" "));

    // After a wide cluster the mark lands on the base cell, not the continuation.
    QCOMPARE(buffer.setGrapheme(1, 0, "\xE4\xB8\xAD", 2), 2);
    QCOMPARE(buffer.setGrapheme(3, 0, "\xCC\x81", 0), 0);
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray("\xE4\xB8\xAD\xCC\x81"));
    QVERIFY(buffer.cellAt(2, 0).character().isEmpty());
}

void TestTextGridBuffer::testSetGraphemeClipsAtRightEdge() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    QCOMPARE(buffer.setGrapheme(3, 0, "\xE4\xB8\xAD", 2), -1);
    QCOMPARE(buffer.cellAt(3, 0).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.setGrapheme(3, 0, "x", 1), 1);
    QCOMPARE(buffer.setGrapheme(4, 0, "y", 1), -1);
}

void TestTextGridBuffer::testSetGraphemeTransparentBgPreservesUnderlying() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    buffer.fillRect(0, 0, 4, 1, " ", 0xFFFFFFFF, 0xFF112233);
    QCOMPARE(buffer.setGrapheme(1, 0, "\xE4\xB8\xAD", 2, 0xFFABCDEF, 0x00000000), 2);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0xFF112233u);
    QCOMPARE(buffer.cellAt(2, 0).bgColor, 0xFF112233u);
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0xFFABCDEFu);
}

void TestTextGridBuffer::testSetGraphemeOutsideBufferAdvances() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 2));
    QCOMPARE(buffer.setGrapheme(-1, 0, "x", 1), 1);
    QCOMPARE(buffer.setGrapheme(0, -1, "x", 1), 1);
    QCOMPARE(buffer.setGrapheme(0, 2, "x", 1), 1);
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 2; ++y)
            QCOMPARE(buffer.cellAt(x, y).character().toByteArray(), QByteArray(" "));
}

void TestTextGridBuffer::testDiffUsesEraseToEndOfLine() {
    // Clearing a row emits erase-to-end-of-line instead of a run of blanks,
    // with the background reset so the erased cells take the default colour.
    TextGridBuffer before, after;
    QVERIFY(before.resize(80, 2));
    QVERIFY(after.resize(80, 2));
    before.fillRect(0, 0, 80, 1, "X", 0xFFFFFFFF, 0xFF0000FF);
    before.fillRect(10, 1, 20, 1, "Y");
    after.writeText(0, 1, QStringLiteral("abc"));

    const std::string diff = before.diff(after);
    QVERIFY(diff.find("\033[K") != std::string::npos);
    QVERIFY(diff.find(std::string(20, ' ')) == std::string::npos);
    QVERIFY(diff.size() < 120);
    // Row 0 was blue-backed: the erase must not inherit that background.
    QVERIFY(diff.find("\033[0m") != std::string::npos);
    QVERIFY(diff.find("abc") != std::string::npos);
    QCOMPARE(occurrenceCount(diff, "\033[K"), 2);   // one per row

    AnsiEncodeOptions noErase;
    noErase.eraseToEndOfLine = false;
    std::string plain;
    before.diff(after, plain, noErase);
    QVERIFY(plain.find("\033[K") == std::string::npos);
    QVERIFY(plain.find(std::string(20, ' ')) != std::string::npos);
}

void TestTextGridBuffer::testDiffEraseRespectsMinimum() {
    TextGridBuffer before, after;
    QVERIFY(before.resize(3, 1));
    QVERIFY(after.resize(3, 1));
    before.fillRect(0, 0, 3, 1, "X");
    const std::string diff = before.diff(after);
    QVERIFY(diff.find("\033[K") == std::string::npos);
    QVERIFY(diff.find("   ") != std::string::npos);
}

void TestTextGridBuffer::testDiffEraseNotUsedWhenTailStyled() {
    TextGridBuffer before, after;
    QVERIFY(before.resize(10, 1));
    QVERIFY(after.resize(10, 1));
    before.fillRect(0, 0, 10, 1, "X");
    after.fillRect(0, 0, 10, 1, " ", 0xFFFFFFFF, 0xFF102030);   // blank but coloured
    const std::string diff = before.diff(after);
    QVERIFY(diff.find("\033[K") == std::string::npos);
    QVERIFY(diff.find(std::string(10, ' ')) != std::string::npos);
    QVERIFY(diff.find("48;2;16;32;48") != std::string::npos);
}

void TestTextGridBuffer::testDiffAppendsIntoCallerString() {
    TextGridBuffer before, after;
    QVERIFY(before.resize(4, 1));
    QVERIFY(after.resize(4, 1));
    after.writeText(0, 0, QStringLiteral("hi"));
    std::string out = "prefix";
    before.diff(after, out);
    QVERIFY(out.starts_with("prefix"));
    QCOMPARE(out.substr(6), before.diff(after));
}

void TestTextGridBuffer::testDiffLargeCoordinates() {
    TextGridBuffer before, after;
    QVERIFY(before.resize(120, 100));
    QVERIFY(after.resize(120, 100));
    after.writeText(119, 99, QStringLiteral("Z"));
    const std::string diff = before.diff(after);
    QVERIFY(diff.find("\033[100;120H") != std::string::npos);
    QVERIFY(diff.ends_with("Z"));
}

QTEST_MAIN(TestTextGridBuffer)
#include "test_text_grid_buffer.moc"
