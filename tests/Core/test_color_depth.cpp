// Colour quantisation and the colour-depth aware ANSI encoder.
#include <QObject>
#include <QTest>
#include <string>

#include <Qtui/Core/primitives/ColorMapper.h>
#include <Qtui/Core/primitives/AnsiEncoder_p.h>
#include <Qtui/Core/primitives/TextGridBuffer.h>

using namespace qtui;

namespace {

int occurrenceCount(const std::string &text, const std::string &needle)
{
    int count = 0;
    size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

std::string encode(ColorDepth depth, void (*fill)(TextGridBuffer &))
{
    TextGridBuffer before;
    TextGridBuffer after;
    before.resize(6, 1);
    after.resize(6, 1);
    fill(after);
    AnsiEncodeOptions options;
    options.colorDepth = depth;
    std::string out;
    before.diff(after, out, options);
    return out;
}

void redOnNavy(TextGridBuffer &buffer)
{
    buffer.writeText(0, 0, QStringLiteral("AB"), 0xFFFF0000, 0xFF000080, TextAttribute::Bold);
}

// The same look, but written with ANSI names: yellow text on a navy background.
void namedYellowOnNavy(TextGridBuffer &buffer)
{
    buffer.writeText(0, 0, QStringLiteral("AB"), Color("yellow").argb32(), Color("navy").argb32(),
                     TextAttribute::None);
}

} // namespace

class TestColorDepth : public QObject {
    Q_OBJECT

private slots:
    void testIndexed256Anchors();
    void testBasic16Anchors();
    void testTrueColorEncoding();
    void testIndexed256Encoding();
    void testBasic16Encoding();
    void testMonoEncoding();
    void testQuantisedColoursDeduplicate();
    void testAnsiNamesParseToIndices();
    void testAnsiNamesKeepIndexAtEveryDepth();
    void testDimAndBoldTransitions();
    void testDefaultAndExplicitWhite_data();
    void testDefaultAndExplicitWhite();
};

void TestColorDepth::testDefaultAndExplicitWhite_data()
{
    QTest::addColumn<ColorDepth>("depth");
    QTest::addColumn<QByteArray>("whiteParameter");
    QTest::newRow("truecolor") << ColorDepth::TrueColor << QByteArray("38;2;255;255;255;");
    QTest::newRow("indexed") << ColorDepth::Indexed256 << QByteArray("38;5;231;");
    QTest::newRow("basic") << ColorDepth::Basic16 << QByteArray("97;");
    QTest::newRow("mono") << ColorDepth::Mono << QByteArray();
}

void TestColorDepth::testDefaultAndExplicitWhite()
{
    QFETCH(ColorDepth, depth);
    QFETCH(QByteArray, whiteParameter);
    TextGridBuffer before, after;
    before.resize(4, 1);
    after.resize(4, 1);
    // Styled spaces must use terminal colors and must not be erased away.
    after.cellAt(0, 0).attributes = TextAttribute::Underline;
    after.cellAt(1, 0).attributes = TextAttribute::Reversed;
    after.writeText(2, 0, QStringLiteral("W"), 0xFFFFFFFF);
    after.writeText(3, 0, QStringLiteral("D"));
    AnsiEncodeOptions options;
    options.colorDepth = depth;
    for (bool relative : {false, true}) {
        std::string out;
        if (relative) before.diffRelative(after, out, options);
        else before.diff(after, out, options);
        const std::string expected = "\033[0m\033[4m \033[24;7m \033["
            + whiteParameter.toStdString() + "27mW"
            + (depth == ColorDepth::Mono ? "" : "\033[39m") + "D";
        QVERIFY2(out.ends_with(expected), QByteArray::fromStdString(out).constData());
    }
    if (depth == ColorDepth::TrueColor) {
        QCOMPARE(after.toAnsiString(), std::string(
            "\033[0m\033[4m \033[0m\033[7m "
            "\033[0m\033[38;2;255;255;255mW\033[0mD\033[0m"));
    }
}

void TestColorDepth::testDimAndBoldTransitions()
{
    ansi::StyleState state;
    Cell cell;
    cell.fgColor = 0;
    cell.bgColor = 0;
    std::string out;
    cell.attributes = TextAttribute::Bold | TextAttribute::Dim;
    ansi::appendStyleTransition(out, cell, state, ColorDepth::TrueColor);
    QVERIFY(out.find("\033[1;2m") != std::string::npos);
    out.clear();
    cell.attributes = TextAttribute::Bold;
    ansi::appendStyleTransition(out, cell, state, ColorDepth::TrueColor);
    QCOMPARE(out, std::string("\033[22;1m"));
    out.clear();
    cell.attributes = TextAttribute::Dim;
    ansi::appendStyleTransition(out, cell, state, ColorDepth::TrueColor);
    QCOMPARE(out, std::string("\033[22;2m"));
    out.clear();
    cell.attributes = TextAttribute::None;
    ansi::appendStyleTransition(out, cell, state, ColorDepth::TrueColor);
    QCOMPARE(out, std::string("\033[22m"));
}

void TestColorDepth::testIndexed256Anchors()
{
    QCOMPARE(ColorMapper::toIndexed256(0xFF000000), uint8_t(16));
    QCOMPARE(ColorMapper::toIndexed256(0xFFFFFFFF), uint8_t(231));
    QCOMPARE(ColorMapper::toIndexed256(0xFF808080), uint8_t(244));
    QCOMPARE(ColorMapper::toIndexed256(0xFFFF0000), uint8_t(196));
    QCOMPARE(ColorMapper::toIndexed256(0xFF00FF00), uint8_t(46));
    QCOMPARE(ColorMapper::toIndexed256(0xFF0000FF), uint8_t(21));
    QCOMPARE(ColorMapper::toIndexed256(0xFF5F87AF), uint8_t(67));   // exact cube colour
}

void TestColorDepth::testBasic16Anchors()
{
    QCOMPARE(ColorMapper::toBasic16(0xFF000000), uint8_t(0));
    QCOMPARE(ColorMapper::toBasic16(0xFFFFFFFF), uint8_t(15));
    QCOMPARE(ColorMapper::toBasic16(0xFF808080), uint8_t(8));
    QCOMPARE(ColorMapper::toBasic16(0xFFFF0000), uint8_t(9));
    QCOMPARE(ColorMapper::toBasic16(0xFF000080), uint8_t(4));
    QCOMPARE(ColorMapper::toBasic16(0xFFC0C0C0), uint8_t(7));
}

void TestColorDepth::testTrueColorEncoding()
{
    const std::string out = encode(ColorDepth::TrueColor, redOnNavy);
    QVERIFY(out.find("38;2;255;0;0") != std::string::npos);
    QVERIFY(out.find("48;2;0;0;128") != std::string::npos);
    QVERIFY(out.find("38;5;") == std::string::npos);
}

void TestColorDepth::testIndexed256Encoding()
{
    const std::string out = encode(ColorDepth::Indexed256, redOnNavy);
    QVERIFY(out.find("38;5;196") != std::string::npos);
    QVERIFY(out.find("48;5;") != std::string::npos);
    QVERIFY(out.find("38;2;") == std::string::npos);
    QVERIFY(out.find("AB") != std::string::npos);
}

void TestColorDepth::testBasic16Encoding()
{
    const std::string out = encode(ColorDepth::Basic16, redOnNavy);
    QVERIFY(out.find("\033[91;44;1m") != std::string::npos);   // bright red on navy, bold
    QVERIFY(out.find("38;") == std::string::npos);
    QVERIFY(out.find("48;") == std::string::npos);
}

void TestColorDepth::testMonoEncoding()
{
    const std::string out = encode(ColorDepth::Mono, redOnNavy);
    QVERIFY(out.find("38;") == std::string::npos);
    QVERIFY(out.find("48;") == std::string::npos);
    QVERIFY(out.find("\033[1m") != std::string::npos);        // attributes survive
    QVERIFY(out.find("AB") != std::string::npos);
}

void TestColorDepth::testQuantisedColoursDeduplicate()
{
    // Two reds that map to the same palette index produce one colour change.
    const std::string out = encode(ColorDepth::Indexed256, [](TextGridBuffer &buffer) {
        buffer.writeText(0, 0, QStringLiteral("A"), 0xFFFF0000);
        buffer.writeText(1, 0, QStringLiteral("B"), 0xFFFE0101);
    });
    QCOMPARE(occurrenceCount(out, "38;5;196"), 1);

    const std::string trueColor = encode(ColorDepth::TrueColor, [](TextGridBuffer &buffer) {
        buffer.writeText(0, 0, QStringLiteral("A"), 0xFFFF0000);
        buffer.writeText(1, 0, QStringLiteral("B"), 0xFFFE0101);
    });
    QCOMPARE(occurrenceCount(trueColor, "38;2;"), 2);
}

QTEST_MAIN(TestColorDepth)
#include "test_color_depth.moc"

void TestColorDepth::testAnsiNamesParseToIndices()
{
    const Color red("red");
    QVERIFY(red.isValid());
    QVERIFY(red.isAnsiIndex());
    QCOMPARE(red.ansiIndex(), 9);
    QCOMPARE(red.toString(), QStringLiteral("red"));
    QCOMPARE(red.toRgb().argb32(), 0xFFFF0000U);          // xterm default for index 9
    QCOMPARE(Color("gray").ansiIndex(), 8);
    QCOMPARE(Color("ansi:3").ansiIndex(), 3);
    QVERIFY(!Color("#ff0000").isAnsiIndex());             // hex stays literal RGB
    QVERIFY(!Color("transparent").isAnsiIndex());
    QVERIFY(!Color("orange").isAnsiIndex());              // not a palette entry
    QCOMPARE(ColorMapper::toBasic16(red.argb32()), uint8_t(9));
    QCOMPARE(ColorMapper::toArgb(red.argb32()), 0xFFFF0000U);
}

void TestColorDepth::testAnsiNamesKeepIndexAtEveryDepth()
{
    // Truecolor: an ANSI name must not be turned into 38;2;R;G;B, or the
    // terminal theme cannot apply. Index 11 is bright yellow (93), navy is 4 (44).
    const std::string trueColor = encode(ColorDepth::TrueColor, namedYellowOnNavy);
    QVERIFY(trueColor.find("93;44") != std::string::npos);
    QVERIFY(trueColor.find("38;2;") == std::string::npos);
    QVERIFY(trueColor.find("48;2;") == std::string::npos);

    const std::string indexed = encode(ColorDepth::Indexed256, namedYellowOnNavy);
    QVERIFY(indexed.find("93;44") != std::string::npos);
    QVERIFY(indexed.find("38;5;") == std::string::npos);

    const std::string basic = encode(ColorDepth::Basic16, namedYellowOnNavy);
    QVERIFY(basic.find("93;44") != std::string::npos);

    const std::string mono = encode(ColorDepth::Mono, namedYellowOnNavy);
    QVERIFY(mono.find("93") == std::string::npos);
}
