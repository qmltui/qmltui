#include <QtTest/QtTest>
#include <Qamui/Core/primitives/Subpixel.h>

using namespace qamui;

class TestSubpixel : public QObject {
    Q_OBJECT

private slots:
    void testDotMask();
    void testUtf8AllMasks();
    void testBlockGlyphs();
    void testDimensions();
};

void TestSubpixel::testDotMask() {
    // Standard 8-dot Braille mapping (U+2800..U+28FF)
    // Left column (x=0): dot 1 (y=0) -> bit 0 (0x01), dot 2 (y=1) -> bit 1 (0x02),
    //                    dot 3 (y=2) -> bit 2 (0x04), dot 7 (y=3) -> bit 6 (0x40)
    QCOMPARE(braille::dotMask(0, 0), static_cast<uint8_t>(0x01));
    QCOMPARE(braille::dotMask(0, 1), static_cast<uint8_t>(0x02));
    QCOMPARE(braille::dotMask(0, 2), static_cast<uint8_t>(0x04));
    QCOMPARE(braille::dotMask(0, 3), static_cast<uint8_t>(0x40));

    // Right column (x=1): dot 4 (y=0) -> bit 3 (0x08), dot 5 (y=1) -> bit 4 (0x10),
    //                     dot 6 (y=2) -> bit 5 (0x20), dot 8 (y=3) -> bit 7 (0x80)
    QCOMPARE(braille::dotMask(1, 0), static_cast<uint8_t>(0x08));
    QCOMPARE(braille::dotMask(1, 1), static_cast<uint8_t>(0x10));
    QCOMPARE(braille::dotMask(1, 2), static_cast<uint8_t>(0x20));
    QCOMPARE(braille::dotMask(1, 3), static_cast<uint8_t>(0x80));
}

void TestSubpixel::testUtf8AllMasks() {
    // Every pattern from 0x00 to 0xFF must match independent Unicode conversion
    for (int mask = 0; mask < 256; ++mask) {
        const QByteArrayView view = braille::utf8(static_cast<uint8_t>(mask));
        QCOMPARE(view.size(), 3);

        const char32_t cp = 0x2800 + mask;
        const QString expectedStr = QString::fromUcs4(&cp, 1);
        const QByteArray expectedBytes = expectedStr.toUtf8();

        QCOMPARE(view.toByteArray(), expectedBytes);
    }

    // Specific boundary cases
    // Mask 0x00: U+2800 (Braille pattern blank) -> \xE2\xA0\x80
    QCOMPARE(braille::utf8(0x00).toByteArray(), QByteArray("\xE2\xA0\x80"));
    // Mask 0xFF: U+28FF (Braille pattern dots 12345678) -> \xE2\xA3\xBF
    QCOMPARE(braille::utf8(0xFF).toByteArray(), QByteArray("\xE2\xA3\xBF"));
}

void TestSubpixel::testBlockGlyphs() {
    // U+2580: Upper Half Block ▀
    const char32_t upper = 0x2580;
    QCOMPARE(QByteArray(block_glyphs::UpperHalf), QString::fromUcs4(&upper, 1).toUtf8());

    // U+2584: Lower Half Block ▄
    const char32_t lower = 0x2584;
    QCOMPARE(QByteArray(block_glyphs::LowerHalf), QString::fromUcs4(&lower, 1).toUtf8());

    // U+2588: Full Block █
    const char32_t full = 0x2588;
    QCOMPARE(QByteArray(block_glyphs::Full), QString::fromUcs4(&full, 1).toUtf8());
}

void TestSubpixel::testDimensions() {
    QCOMPARE(braille::DotsX, 2);
    QCOMPARE(braille::DotsY, 4);
    QCOMPARE(half_block::SubpixelsX, 1);
    QCOMPARE(half_block::SubpixelsY, 2);
}

QTEST_MAIN(TestSubpixel)
#include "test_subpixel.moc"
