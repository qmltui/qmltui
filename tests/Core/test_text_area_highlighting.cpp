#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QElapsedTimer>
#include <QmlTui/Core/Items/TextArea.h>
#include <QmlTui/Core/Primitives/HighlightRange.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include "../TestPaint.h"
#include <climits>
#include <memory>

using namespace qmltui;

static TextGridBuffer frame(TextArea &area, int width = 40, int height = 10)
{
    TextGridBuffer buffer;
    buffer.resize(width, height);
    area.setWidth(width);
    area.setHeight(height);
    testing::paintItem(area, buffer, 0, 0);
    return buffer;
}

class TestTextAreaHighlighting : public QObject
{
    Q_OBJECT
private slots:
    void valuesAndResets();
    void qmlStructuredValue();
    void validationAndRejection();
    void layerPrecedence();
    void graphemeSnapping();
    void textEditInvalidation();
    void rendering();
    void scrolledRendering();
    void transparentHighlights();
    void cursorPropertiesAndRendering();
    void acceptedSignal();
    void tabHandling();
    void performance();
};

void TestTextAreaHighlighting::valuesAndResets()
{
    HighlightRange r;
    QCOMPARE(r.start(), 0);
    QCOMPARE(r.length(), 0);
    QVERIFY(!r.color().isValid());
    QVERIFY(!r.backgroundColor().isValid());
    QVERIFY(!r.hasBold());

    r.setStart(5);
    r.setLength(10);
    r.setColor(Color("#ff0000"));
    r.setBackgroundColor(Color("#00ff00"));
    r.setBold(true);
    r.setItalic(true);
    r.setUnderline(true);
    r.setDim(true);
    r.setReversed(true);

    QCOMPARE(r.start(), 5);
    QCOMPARE(r.length(), 10);
    QCOMPARE(r.color(), Color("#ff0000"));
    QCOMPARE(r.backgroundColor(), Color("#00ff00"));
    QVERIFY(r.bold());
    QVERIFY(r.hasBold());
    QVERIFY(r.italic());
    QVERIFY(r.underline());
    QVERIFY(r.dim());
    QVERIFY(r.reversed());

    HighlightRange copy = r;
    QCOMPARE(copy, r);

    r.resetColor();
    r.resetBackgroundColor();
    r.resetBold();
    r.resetItalic();
    r.resetUnderline();
    r.resetDim();
    r.resetReversed();

    QVERIFY(!r.color().isValid());
    QVERIFY(!r.backgroundColor().isValid());
    QVERIFY(!r.hasBold());
    QVERIFY(!r.hasItalic());
    QVERIFY(!r.hasUnderline());
    QVERIFY(!r.hasDim());
    QVERIFY(!r.hasReversed());
    QVERIFY(copy != r);
}

void TestTextAreaHighlighting::qmlStructuredValue()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QmlTui
        TextArea {
            text: "Hello world QML"
            function applyHighlights(): bool {
                return setHighlightLayer("syntax", [
                    { start: 0, length: 5, color: "#ff0000", bold: true },
                    { start: 6, length: 5, backgroundColor: "#0000ff" }
                ], textRevision)
            }
        }
    )", QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *area = qobject_cast<TextArea *>(object.get());
    QVERIFY(area);

    bool result = false;
    QVERIFY(QMetaObject::invokeMethod(area, "applyHighlights", Q_RETURN_ARG(bool, result)));
    QVERIFY(result);

    auto buffer = frame(*area);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(6, 0).bgColor, 0xff0000ffu);
}

void TestTextAreaHighlighting::validationAndRejection()
{
    TextArea area;
    area.setText(QStringLiteral("0123456789"));
    const quint64 rev = area.textRevision();

    // Valid range
    HighlightRange validRange(0, 5, Color("red"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("layer1"), {validRange}, rev));

    // Stale revision
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {validRange}, rev - 1));

    // Future revision
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {validRange}, rev + 1));

    // Empty layer name
    QVERIFY(!area.setHighlightLayer(QString(), {validRange}, rev));

    // Negative start
    HighlightRange negStart(-1, 5);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {negStart}, rev));

    // Negative length
    HighlightRange negLength(2, -1);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {negLength}, rev));

    // Length exceeding bounds
    HighlightRange outOfBounds(5, 6); // 5 + 6 = 11 > 10
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {outOfBounds}, rev));

    // Extreme integer values (overflow safety tests)
    HighlightRange intMaxStart(INT_MAX, 1);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {intMaxStart}, rev));

    HighlightRange intMaxLength(1, INT_MAX);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {intMaxLength}, rev));

    HighlightRange intMinStart(INT_MIN, 1);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {intMinStart}, rev));

    HighlightRange intMinLength(1, INT_MIN);
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer2"), {intMinLength}, rev));

    // Atomic rejection: list with valid followed by invalid should not modify existing layer
    HighlightRange validRange2(5, 5, Color("blue"));
    QVERIFY(!area.setHighlightLayer(QStringLiteral("layer1"), {validRange2, intMaxLength}, rev));

    // Verify layer1 is still intact from the first valid call
    auto buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("red").argb32());

    // Zero-length range is valid but produces no visual cell styling
    HighlightRange zeroLen(2, 0, Color("green"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("zero"), {zeroLen}, rev));
}

void TestTextAreaHighlighting::layerPrecedence()
{
    TextArea area;
    area.setText(QStringLiteral("abcdefghij"));
    const quint64 rev = area.textRevision();

    // Layer A: priority 0, syntax (fg red, bold true)
    HighlightRange rangeA(0, 10, Color("red"));
    rangeA.setBold(true);
    QVERIFY(area.setHighlightLayer(QStringLiteral("syntax"), {rangeA}, rev, 0));

    // Layer B: priority 10, search (bg yellow) -> should override only bg, keep red fg and bold
    HighlightRange rangeB(2, 5, Color(), Color("yellow"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("search"), {rangeB}, rev, 10));

    auto buffer = frame(area);
    // At index 0: syntax only
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("red").argb32());
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0u);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);

    // At index 2: search background + syntax foreground and bold cascade
    QCOMPARE(buffer.cellAt(2, 0).fgColor, Color("red").argb32());
    QCOMPARE(buffer.cellAt(2, 0).bgColor, Color("yellow").argb32());
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Bold);

    // Equal priority name ordering: later name in alphabetical order wins ("beta" > "alpha")
    HighlightRange rangeAlpha(0, 5, Color("blue"));
    HighlightRange rangeBeta(0, 5, Color("cyan"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("alpha"), {rangeAlpha}, rev, 5));
    QVERIFY(area.setHighlightLayer(QStringLiteral("beta"), {rangeBeta}, rev, 5));

    buffer = frame(area);
    // beta wins over alpha
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("cyan").argb32());

    // Within single layer: later entries in range list win
    HighlightRange first(0, 4, Color("green"));
    HighlightRange second(2, 2, Color("magenta"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("single"), {first, second}, rev, 20));

    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("green").argb32());
    QCOMPARE(buffer.cellAt(2, 0).fgColor, Color("magenta").argb32());
}

void TestTextAreaHighlighting::graphemeSnapping()
{
    TextArea area;
    // Combining character: 'e' + combining acute (U+0301) at index 0..2
    area.setText(QString::fromUtf8("e\xcc\x81xyz"));
    const quint64 rev = area.textRevision();

    // Partial range covering only the base 'e' (start 0, len 1)
    HighlightRange subCluster(0, 1, Color("red"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("snap"), {subCluster}, rev));

    auto buffer = frame(area);
    // Cell 0 should contain the whole grapheme cluster and have red color
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("red").argb32());
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("e\xcc\x81"));
    // Following characters ('x') not highlighted
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("x"));
    QCOMPARE(buffer.cellAt(1, 0).fgColor, area.color().argb32());
}

void TestTextAreaHighlighting::textEditInvalidation()
{
    TextArea area;
    area.setText(QStringLiteral("Initial text"));
    QSignalSpy spyRev(&area, &TextArea::textRevisionChanged);
    QSignalSpy spyHigh(&area, &TextArea::highlightsChanged);

    HighlightRange range(0, 7, Color("red"));
    QVERIFY(area.setHighlightLayer(QStringLiteral("test"), {range}, area.textRevision()));
    QCOMPARE(spyHigh.count(), 1);

    // Setting identical layer contents is a no-op: returns true, no highlightsChanged
    QVERIFY(area.setHighlightLayer(QStringLiteral("test"), {range}, area.textRevision()));
    QCOMPARE(spyHigh.count(), 1);

    // Modify text: revision increments, highlights cleared, highlightsChanged emitted once
    area.setText(QStringLiteral("Changed text"));
    QCOMPARE(spyRev.count(), 1);
    QCOMPARE(spyHigh.count(), 2);

    // Modify text again when highlights were already empty: highlightsChanged NOT emitted
    area.setText(QStringLiteral("Changed again"));
    QCOMPARE(spyRev.count(), 2);
    QCOMPARE(spyHigh.count(), 2); // No extra highlightsChanged emission

    // Synchronous rehighlighting during textChanged
    bool synchronousRehighlightSuccess = false;
    QObject::connect(&area, &TextArea::textChanged, [&]() {
        HighlightRange newRange(0, 5, Color("blue"));
        synchronousRehighlightSuccess = area.setHighlightLayer(
            QStringLiteral("sync"), {newRange}, area.textRevision());
    });

    area.setText(QStringLiteral("Fresh document content"));
    QVERIFY(synchronousRehighlightSuccess);
    auto buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("blue").argb32());

    // Cursor position changes do NOT invalidate highlights or increment revision
    const quint64 revBeforeCursor = area.textRevision();
    area.setCursorPosition(2);
    QCOMPARE(area.textRevision(), revBeforeCursor);
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("blue").argb32());
}

void TestTextAreaHighlighting::rendering()
{
    TextArea area;
    area.setText(QStringLiteral("Line1\nLine2\nLine3"));
    const quint64 rev = area.textRevision();

    // Multiline highlight range covering parts of Line1 and Line2
    // "Line1\n" is 6 chars. Line2 starts at 6.
    HighlightRange multi(2, 7, Color("blue")); // covers "ne1\nLi"
    multi.setUnderline(true);
    QVERIFY(area.setHighlightLayer(QStringLiteral("multi"), {multi}, rev));

    auto buffer = frame(area);
    // Line 1: 'n' at column 2
    QCOMPARE(buffer.cellAt(2, 0).character(), QByteArrayView("n"));
    QCOMPARE(buffer.cellAt(2, 0).fgColor, Color("blue").argb32());
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Underline);

    // Line 2: 'L' at column 0, 'i' at column 1
    QCOMPARE(buffer.cellAt(0, 1).character(), QByteArrayView("L"));
    QCOMPARE(buffer.cellAt(0, 1).fgColor, Color("blue").argb32());
    QCOMPARE(buffer.cellAt(0, 1).attributes, TextAttribute::Underline);

    // Selection reversal preserves highlight colors
    area.selectAll();
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(2, 0).fgColor, Color("blue").argb32());
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Underline | TextAttribute::Reversed);
    area.clearSelection();

    // Disabled state preserves highlight colors and adds Dim
    area.setEnabled(false);
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(2, 0).fgColor, Color("blue").argb32());
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Underline | TextAttribute::Dim);
    area.setEnabled(true);

    // Placeholder text receives NO document highlights
    TextArea placeholderArea;
    placeholderArea.setPlaceholderText(QStringLiteral("Enter text here..."));
    // text is empty
    HighlightRange phRange(0, 10, Color("red"));
    // submitting for empty text with length > 0 is rejected (0 + 10 > 0)
    QVERIFY(!placeholderArea.setHighlightLayer(QStringLiteral("h"), {phRange}, placeholderArea.textRevision()));
    buffer = frame(placeholderArea);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("E"));
    // Default placeholder styling (Dim), not red
    QVERIFY(buffer.cellAt(0, 0).fgColor != Color("red").argb32());
}

void TestTextAreaHighlighting::scrolledRendering()
{
    TextArea area;
    area.setText(QStringLiteral("Row0-alpha\nRow1-beta\nRow2-gamma\nRow3-delta"));
    area.setWidth(40);
    area.setHeight(2);
    const quint64 rev = area.textRevision();

    // Highlight on Row 0 (hidden when scrolled) and Row 2 (visible when scrolled)
    // Row 0: "Row0-alpha\n" -> 11 chars
    // Row 1: "Row1-beta\n"  -> 10 chars (start 11)
    // Row 2: "Row2-gamma\n" -> 11 chars (start 21)
    HighlightRange r0(0, 4, Color("red")); // "Row0"
    HighlightRange r2(21, 4, Color("green")); // "Row2"
    QVERIFY(area.setHighlightLayer(QStringLiteral("hl"), {r0, r2}, rev));

    // Scroll down by 2 lines: Row 2 is now at visible row y=0
    area.setContentY(2);
    auto buffer = frame(area, 40, 2);

    // Row 2 is rendered at visible y = 0
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("R"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, Color("green").argb32());
    // "gamma" at column 5 on visible y = 0 is unhighlighted default text
    QCOMPARE(buffer.cellAt(5, 0).character(), QByteArrayView("g"));
    QCOMPARE(buffer.cellAt(5, 0).fgColor, area.color().argb32());

    // Row 3 is rendered at visible y = 1
    QCOMPARE(buffer.cellAt(0, 1).character(), QByteArrayView("R"));
    QCOMPARE(buffer.cellAt(0, 1).fgColor, area.color().argb32());
}

void TestTextAreaHighlighting::transparentHighlights()
{
    TextArea area;
    area.setText(QStringLiteral("Transparent test"));
    const quint64 rev = area.textRevision();

    HighlightRange transRange(0, 11, Color::transparent(), Color::transparent());
    transRange.setBold(true);
    QVERIFY(area.setHighlightLayer(QStringLiteral("trans"), {transRange}, rev));

    auto buffer = frame(area);
    // Cells must not contain the internal transparent tag (0x04000000)
    QVERIFY(buffer.cellAt(0, 0).fgColor != Color::TransparentTag);
    QVERIFY(buffer.cellAt(0, 0).bgColor != Color::TransparentTag);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0u);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
}

void TestTextAreaHighlighting::cursorPropertiesAndRendering()
{
    TextArea area;
    QSignalSpy spyShape(&area, &TextArea::cursorShapeChanged);
    QSignalSpy spyColor(&area, &TextArea::cursorColorChanged);
    QSignalSpy spyVisible(&area, &TextArea::cursorVisibleChanged);

    // Defaults
    QCOMPARE(area.cursorShape(), TextArea::Block);
    QCOMPARE(area.cursorColor(), Color());
    QVERIFY(!area.cursorColor().isValid());
    QCOMPARE(area.cursorVisible(), true);

    // Setters and signals
    area.setCursorShape(TextArea::Beam);
    QCOMPARE(area.cursorShape(), TextArea::Beam);
    QCOMPARE(spyShape.count(), 1);

    area.setCursorColor(Color("cyan"));
    QCOMPARE(area.cursorColor(), Color("cyan"));
    QCOMPARE(spyColor.count(), 1);

    area.setCursorVisible(false);
    QCOMPARE(area.cursorVisible(), false);
    QCOMPARE(spyVisible.count(), 1);

    // Unfocused: no cursor rendered even if active
    area.setText(QStringLiteral("a b"));
    area.setCursorPosition(0);
    area.setActiveFocus(false);
    area.setCursorVisible(true);
    auto buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::None);

    // Focused with Block cursor (default color): reverses cell attributes
    area.setActiveFocus(true);
    area.setCursorShape(TextArea::Block);
    area.setCursorColor(Color("default"));
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("a"));
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Reversed);

    // Focused with Block cursor and explicit color: sets bgColor to cursorColor and fgColor to black
    area.setCursorColor(Color("#00ffff")); // cyan
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xff00ffffu);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xff000000u);

    // Cursor inside selection: toggles Reversed so cursor remains distinct
    area.setCursorColor(Color("default"));
    area.setCursorPosition(2);
    KeyEvent shiftLeft(Qt::Key_Left, "", Qt::ShiftModifier);
    area.keyPressEvent(&shiftLeft);
    QCOMPARE(area.selectionStart(), 1);
    QCOMPARE(area.selectionEnd(), 2);
    QCOMPARE(area.cursorPosition(), 1);
    buffer = frame(area);
    // Over the cursor cell (1, 0), Reversed attribute is toggled off (0)
    QCOMPARE(buffer.cellAt(1, 0).attributes & TextAttribute::Reversed, TextAttribute::None);
    area.clearSelection();

    // Beam cursor: on space renders '│', over character renders as block
    area.setCursorShape(TextArea::Beam);
    area.setCursorPosition(1); // at the space in "a b"
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("\xe2\x94\x82"));

    area.setCursorPosition(0); // over 'a'
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("a"));
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Reversed);

    // Underline cursor: on space renders '_', over character underlines it
    area.setCursorShape(TextArea::Underline);
    area.setCursorPosition(1); // space
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("_"));

    area.setCursorPosition(0); // over 'a'
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("a"));
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Underline);

    // Over an already-underlined character, Underline cursor falls back to block (Reversed)
    HighlightRange ul(0, 1, Color());
    ul.setUnderline(true);
    QVERIFY(area.setHighlightLayer(QStringLiteral("ul"), {ul}, area.textRevision()));
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).attributes & TextAttribute::Reversed, TextAttribute::Reversed);
    area.clearHighlights();

    // HollowBlock cursor: on space renders '▯', over character underlines it
    area.setCursorShape(TextArea::HollowBlock);
    area.setCursorPosition(1); // space
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("\xe2\x96\xaf"));

    area.setCursorPosition(0); // over 'a'
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("a"));
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Underline);

    // cursorVisible false: no cursor painted
    area.setCursorVisible(false);
    buffer = frame(area);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::None);
}

void TestTextAreaHighlighting::acceptedSignal()
{
    TextArea area;
    area.setActiveFocus(true);
    QSignalSpy spy(&area, &TextArea::accepted);

    // Regular Enter inserts newline, does not emit accepted
    KeyEvent enterEv(Qt::Key_Return, "\n", Qt::NoModifier);
    area.keyPressEvent(&enterEv);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(area.text(), QStringLiteral("\n"));

    // Ctrl+Return emits accepted
    KeyEvent ctrlReturnEv(Qt::Key_Return, "\n", Qt::ControlModifier);
    area.keyPressEvent(&ctrlReturnEv);
    QCOMPARE(spy.count(), 1);
    QVERIFY(ctrlReturnEv.isAccepted());

    // Ctrl+Enter emits accepted
    KeyEvent ctrlEnterEv(Qt::Key_Enter, "\n", Qt::ControlModifier);
    area.keyPressEvent(&ctrlEnterEv);
    QCOMPARE(spy.count(), 2);
    QVERIFY(ctrlEnterEv.isAccepted());
}

void TestTextAreaHighlighting::tabHandling()
{
    TextArea area;
    area.setActiveFocus(true);
    QSignalSpy spyFocusChange(&area, &TextArea::tabChangesFocusChanged);
    QSignalSpy spyDistanceChange(&area, &TextArea::tabStopDistanceChanged);
    QSignalSpy spyTextChanged(&area, &TextArea::textChanged);

    // 1. Defaults
    QCOMPARE(area.tabChangesFocus(), false);
    QCOMPARE(area.tabStopDistance(), 4);

    // Setters and signals
    area.setTabChangesFocus(true);
    QCOMPARE(area.tabChangesFocus(), true);
    QCOMPARE(spyFocusChange.count(), 1);

    area.setTabStopDistance(2);
    QCOMPARE(area.tabStopDistance(), 2);
    QCOMPARE(spyDistanceChange.count(), 1);

    // Sanitization: cannot be less than 1
    area.setTabStopDistance(0);
    QCOMPARE(area.tabStopDistance(), 1);

    // Reset for tab input testing
    area.setTabChangesFocus(false);
    area.setTabStopDistance(4);

    // 2. Tab insertion at col 0 inserts 4 spaces and accepts event
    KeyEvent tabEv(Qt::Key_Tab, "", Qt::NoModifier);
    area.keyPressEvent(&tabEv);
    QVERIFY(tabEv.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("    "));
    QCOMPARE(area.cursorPosition(), 4);
    QCOMPARE(spyTextChanged.count(), 1);

    // 3. Tab insertion at col 4 inserts 4 more spaces
    KeyEvent tabEv2(Qt::Key_Tab, "", Qt::NoModifier);
    area.keyPressEvent(&tabEv2);
    QVERIFY(tabEv2.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("        "));
    QCOMPARE(area.cursorPosition(), 8);

    // 4. Backtab unindents 4 spaces back to col 4
    KeyEvent backtabEv(Qt::Key_Backtab, "", Qt::ShiftModifier);
    area.keyPressEvent(&backtabEv);
    QVERIFY(backtabEv.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("    "));
    QCOMPARE(area.cursorPosition(), 4);

    // 5. Another Backtab unindents back to col 0
    KeyEvent backtabEv2(Qt::Key_Backtab, "", Qt::ShiftModifier);
    area.keyPressEvent(&backtabEv2);
    QVERIFY(backtabEv2.isAccepted());
    QCOMPARE(area.text(), QStringLiteral(""));
    QCOMPARE(area.cursorPosition(), 0);

    // 6. Backtab at col 0 has nothing to unindent: leaves event unaccepted so InputRouter can cycle focus backward
    KeyEvent backtabEv3(Qt::Key_Backtab, "", Qt::ShiftModifier);
    area.keyPressEvent(&backtabEv3);
    QVERIFY(!backtabEv3.isAccepted());

    // 7. Align to next tab stop (e.g. typing 2 chars, then Tab inserts 2 spaces to reach 4)
    area.setText(QStringLiteral("ab"));
    area.setCursorPosition(2);
    KeyEvent tabAlignEv(Qt::Key_Tab, "", Qt::NoModifier);
    area.keyPressEvent(&tabAlignEv);
    QVERIFY(tabAlignEv.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("ab  "));
    QCOMPARE(area.cursorPosition(), 4);

    // 8. tabChangesFocus == true: Tab leaves event unaccepted, text unchanged
    area.setTabChangesFocus(true);
    KeyEvent tabSkipEv(Qt::Key_Tab, "", Qt::NoModifier);
    area.keyPressEvent(&tabSkipEv);
    QVERIFY(!tabSkipEv.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("ab  "));

    // 9. readOnly == true and tabChangesFocus == false: consumes Tab without altering text
    area.setTabChangesFocus(false);
    area.setReadOnly(true);
    KeyEvent tabRoEv(Qt::Key_Tab, "", Qt::NoModifier);
    area.keyPressEvent(&tabRoEv);
    QVERIFY(tabRoEv.isAccepted());
    QCOMPARE(area.text(), QStringLiteral("ab  "));
}

void TestTextAreaHighlighting::performance()
{
    TextArea area;
    QString longText;
    for (int i = 0; i < 500; ++i) {
        longText += QStringLiteral("token%1 = func%2(arg%3);\n").arg(i).arg(i).arg(i);
    }
    area.setText(longText);
    const quint64 rev = area.textRevision();

    // 1000 ordinary tokens
    QList<HighlightRange> ranges;
    for (int i = 0; i < 1000; ++i) {
        ranges.append(HighlightRange(i * 10, 5, Color("cyan")));
    }

    QElapsedTimer timer;
    timer.start();
    QVERIFY(area.setHighlightLayer(QStringLiteral("perf_syntax"), ranges, rev, 0));
    const qint64 syntaxTimeNs = timer.nsecsElapsed();

    // 1000 deeply nested / overlapping search ranges
    QList<HighlightRange> nestedOverlaps;
    for (int i = 0; i < 1000; ++i) {
        HighlightRange r(i * 2, 50, Color(), Color("yellow"));
        r.setBold(true);
        nestedOverlaps.append(r);
    }
    timer.restart();
    QVERIFY(area.setHighlightLayer(QStringLiteral("perf_search"), nestedOverlaps, rev, 10));
    const qint64 nestedTimeNs = timer.nsecsElapsed();

    // Measure repeated frame repaints: should be extremely fast reusing composed cache
    timer.restart();
    for (int frameCount = 0; frameCount < 20; ++frameCount) {
        frame(area);
    }
    const qint64 repaintTimeNs = timer.nsecsElapsed();

    QVERIFY(syntaxTimeNs > 0);
    QVERIFY(nestedTimeNs > 0);
    QVERIFY(repaintTimeNs < 500'000'000); // 20 frames must take well under 500ms

    qInfo("Performance measurements:\n"
          "  1000 syntax ranges layer: %lld µs\n"
          "  1000 nested overlapping ranges layer: %lld µs\n"
          "  20 viewport repaints: %lld µs (%lld µs/frame)",
          syntaxTimeNs / 1000,
          nestedTimeNs / 1000,
          repaintTimeNs / 1000,
          repaintTimeNs / 20000);
}

QTEST_GUILESS_MAIN(TestTextAreaHighlighting)
#include "test_text_area_highlighting.moc"
