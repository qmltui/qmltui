// Pins grapheme segmentation, display width, and boundary behaviour so the
// UnicodeText implementation can change without altering rendered frames.
#include <QObject>
#include <QTest>
#include <QVector>

#include <Qtui/Core/primitives/UnicodeText.h>

using namespace qtui;

namespace {

QString fromCodePoints(std::initializer_list<char32_t> codePoints)
{
    QString text;
    for (char32_t codePoint : codePoints)
        text.append(QString::fromUcs4(&codePoint, 1));
    return text;
}

void expectSingleCluster(const QString &text, int width, int utf16Length, int utf8Length)
{
    const QVector<Grapheme> clusters = UnicodeText::graphemes(text);
    QCOMPARE(clusters.size(), 1);
    QCOMPARE(clusters[0].utf16Start, 0);
    QCOMPARE(clusters[0].utf16Length, utf16Length);
    QCOMPARE(clusters[0].width, width);
    QCOMPARE(clusters[0].utf8.size(), utf8Length);
    QCOMPARE(clusters[0].utf8, text.toUtf8());
    QCOMPARE(UnicodeText::displayWidth(text), width);
}

} // namespace

class TestUnicodeText : public QObject {
    Q_OBJECT

private slots:
    void testEmptyText();
    void testAsciiClusters();
    void testPrecomposedAndDecomposed();
    void testCjkIsWide();
    void testEmojiWithSkinTone();
    void testVariationSelectorMakesWide();
    void testRegionalIndicatorFlag();
    void testZwjFamilySequence();
    void testKeycapSequence();
    void testZeroWidthSpace();
    void testControlCharactersHaveZeroWidth();
    void testLoneSurrogateBecomesReplacement();
    void testBoundariesRoundTrip();
    void testDisplayWidthSumsClusters();
    void testIteratorMatchesGraphemes();
    void testIteratorSanitizesControls();
    void testEncodeUtf8Capacity();
};

void TestUnicodeText::testEmptyText()
{
    QVERIFY(UnicodeText::graphemes(QString()).isEmpty());
    QCOMPARE(UnicodeText::displayWidth(QString()), 0);
    QCOMPARE(UnicodeText::nextBoundary(QString(), 0), 0);
    QCOMPARE(UnicodeText::previousBoundary(QString(), 0), 0);
}

void TestUnicodeText::testAsciiClusters()
{
    const QVector<Grapheme> clusters = UnicodeText::graphemes(QStringLiteral("abc"));
    QCOMPARE(clusters.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(clusters[i].utf16Start, i);
        QCOMPARE(clusters[i].utf16Length, 1);
        QCOMPARE(clusters[i].width, 1);
        QCOMPARE(clusters[i].utf8, QByteArray(1, static_cast<char>('a' + i)));
    }
    QCOMPARE(UnicodeText::displayWidth(QStringLiteral("abc")), 3);
}

void TestUnicodeText::testPrecomposedAndDecomposed()
{
    expectSingleCluster(fromCodePoints({0x00E9}), 1, 1, 2);           // é
    expectSingleCluster(fromCodePoints({'e', 0x0301}), 1, 2, 3);      // e + combining acute
}

void TestUnicodeText::testCjkIsWide()
{
    expectSingleCluster(fromCodePoints({0x4E2D}), 2, 1, 3);           // 中
}

void TestUnicodeText::testEmojiWithSkinTone()
{
    expectSingleCluster(fromCodePoints({0x1F44D, 0x1F3FD}), 2, 4, 8); // 👍🏽
}

void TestUnicodeText::testVariationSelectorMakesWide()
{
    expectSingleCluster(fromCodePoints({0x2764, 0xFE0F}), 2, 2, 6);   // ❤️
}

void TestUnicodeText::testRegionalIndicatorFlag()
{
    expectSingleCluster(fromCodePoints({0x1F1EB, 0x1F1F7}), 2, 4, 8); // 🇫🇷
}

void TestUnicodeText::testZwjFamilySequence()
{
    // 👨‍👩‍👧: three emoji joined by ZWJ, 18 UTF-8 bytes in one cell.
    expectSingleCluster(fromCodePoints({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467}), 2, 8, 18);
}

void TestUnicodeText::testKeycapSequence()
{
    expectSingleCluster(fromCodePoints({'1', 0xFE0F, 0x20E3}), 2, 3, 7); // 1️⃣
}

void TestUnicodeText::testZeroWidthSpace()
{
    expectSingleCluster(fromCodePoints({0x200B}), 0, 1, 3);
}

void TestUnicodeText::testControlCharactersHaveZeroWidth()
{
    // Raw controls segment as their own zero-width clusters; TextGridBuffer
    // substitutes visible U+24xx glyphs before writing them.
    expectSingleCluster(QString(QChar(0x1b)), 0, 1, 1);
    const QVector<Grapheme> clusters = UnicodeText::graphemes(QStringLiteral("a\x1b" "b"));
    QCOMPARE(clusters.size(), 3);
    QCOMPARE(clusters[1].width, 0);
    QCOMPARE(UnicodeText::displayWidth(QStringLiteral("a\x1b" "b")), 2);
}

void TestUnicodeText::testLoneSurrogateBecomesReplacement()
{
    // QString::toUtf8 encodes a lone surrogate as '?'; it must still be one
    // width-1 cluster rather than corrupting neighbouring clusters.
    const QString text = QString(QChar(0xD800)) + QStringLiteral("a");
    const QVector<Grapheme> clusters = UnicodeText::graphemes(text);
    QCOMPARE(clusters.size(), 2);
    QCOMPARE(clusters[0].width, 1);
    QCOMPARE(clusters[0].utf8, QByteArray("?"));
    QCOMPARE(clusters[1].utf8, QByteArray("a"));
    QCOMPARE(UnicodeText::displayWidth(text), 2);
}

void TestUnicodeText::testBoundariesRoundTrip()
{
    const QString text = QStringLiteral("a") + fromCodePoints({0x1F44D, 0x1F3FD})
                         + QStringLiteral("b") + fromCodePoints({0x4E2D})
                         + fromCodePoints({'e', 0x0301});
    const QVector<Grapheme> clusters = UnicodeText::graphemes(text);
    QCOMPARE(clusters.size(), 5);

    // From any position inside a cluster, nextBoundary is that cluster's end
    // and previousBoundary is the start of the cluster before the position.
    for (const Grapheme &cluster : clusters) {
        const int end = cluster.utf16Start + cluster.utf16Length;
        for (int position = cluster.utf16Start; position < end; ++position)
            QCOMPARE(UnicodeText::nextBoundary(text, position), end);
        QCOMPARE(UnicodeText::previousBoundary(text, end), cluster.utf16Start);
    }
    QCOMPARE(UnicodeText::nextBoundary(text, text.size()), text.size());
    QCOMPARE(UnicodeText::previousBoundary(text, 0), 0);
    QCOMPARE(UnicodeText::nextBoundary(text, -5), clusters[0].utf16Length);
    QCOMPARE(UnicodeText::previousBoundary(text, text.size() + 5), clusters[4].utf16Start);
}

void TestUnicodeText::testDisplayWidthSumsClusters()
{
    const QString text = QStringLiteral("Qt ") + fromCodePoints({0x4E2D, 0x6587})
                         + QStringLiteral(" ") + fromCodePoints({0x1F1EB, 0x1F1F7})
                         + fromCodePoints({0x200B}) + QStringLiteral("!");
    int sum = 0;
    for (const Grapheme &cluster : UnicodeText::graphemes(text))
        sum += cluster.width;
    QCOMPARE(UnicodeText::displayWidth(text), sum);
    QCOMPARE(sum, 3 + 4 + 1 + 2 + 0 + 1);
}

void TestUnicodeText::testIteratorMatchesGraphemes()
{
    const QStringList samples = {
        QString(),
        QStringLiteral("plain ascii"),
        QStringLiteral("Qt ") + fromCodePoints({0x4E2D, 0x6587}) + fromCodePoints({0x1F1EB, 0x1F1F7}),
        fromCodePoints({'e', 0x0301, 0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467, '1', 0xFE0F, 0x20E3, 0x200B}),
        QString(QChar(0xD800)) + QStringLiteral("x") + QString(QChar(0xDC00)),
        QStringLiteral("a\x1b" "b\x7f"),
    };
    for (const QString &sample : samples) {
        const QVector<Grapheme> expected = UnicodeText::graphemes(sample);
        GraphemeIterator iterator(sample);
        GraphemeRef cluster;
        int index = 0;
        while (iterator.next(cluster)) {
            QVERIFY2(index < expected.size(), qPrintable(sample));
            QCOMPARE(cluster.utf16Start, expected[index].utf16Start);
            QCOMPARE(cluster.utf16Length(), expected[index].utf16Length);
            QCOMPARE(cluster.width, expected[index].width);
            QCOMPARE(UnicodeText::toUtf8(cluster.utf16), expected[index].utf8);
            char encoded[64];
            const int length = UnicodeText::encodeUtf8(cluster.utf16, encoded, sizeof encoded);
            QCOMPARE(QByteArray(encoded, length), expected[index].utf8);
            ++index;
        }
        QCOMPARE(index, expected.size());
    }
}

void TestUnicodeText::testIteratorSanitizesControls()
{
    // Sanitised iteration treats controls as their visible replacements, so a
    // combining mark after ESC attaches to U+241B (width 1) instead of to a
    // zero-width control.
    const QString text = QStringLiteral("a\x1b") + fromCodePoints({0x0301}) + QStringLiteral("b");
    GraphemeIterator raw(text);
    GraphemeRef cluster;
    QVERIFY(raw.next(cluster));            // a
    QVERIFY(raw.next(cluster));            // ESC alone: clusters break after controls
    QCOMPARE(cluster.width, 0);
    QCOMPARE(cluster.utf16Length(), 1);
    QVERIFY(raw.next(cluster));            // orphaned combining mark
    QCOMPARE(cluster.width, 0);
    QCOMPARE(cluster.utf16Length(), 1);

    GraphemeIterator sanitized(text, /*sanitizeControls=*/true);
    QVERIFY(sanitized.next(cluster));
    QVERIFY(sanitized.next(cluster));
    QCOMPARE(cluster.width, 1);
    QCOMPARE(cluster.utf16Length(), 2);
    QCOMPARE(UnicodeText::toUtf8(cluster.utf16, true), QByteArray("\xE2\x90\x9B\xCC\x81"));
    QVERIFY(sanitized.next(cluster));
    QCOMPARE(UnicodeText::toUtf8(cluster.utf16, true), QByteArray("b"));
    QVERIFY(!sanitized.next(cluster));

    QCOMPARE(UnicodeText::sanitizeCodePoint(0x7f), char32_t(0x2421));
    QCOMPARE(UnicodeText::sanitizeCodePoint(0x85), char32_t(0xfffd));
    QCOMPARE(UnicodeText::sanitizeCodePoint('A'), char32_t('A'));
}

void TestUnicodeText::testEncodeUtf8Capacity()
{
    const QString family = fromCodePoints({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467});
    char small[8];
    QCOMPARE(UnicodeText::encodeUtf8(family, small, sizeof small), -1);
    char exact[18];
    QCOMPARE(UnicodeText::encodeUtf8(family, exact, sizeof exact), 18);
    QCOMPARE(QByteArray(exact, 18), family.toUtf8());
    QCOMPARE(UnicodeText::toUtf8(family), family.toUtf8());
    QCOMPARE(UnicodeText::encodeUtf8(QString(), exact, sizeof exact), 0);
}

QTEST_MAIN(TestUnicodeText)
#include "test_unicode_text.moc"
