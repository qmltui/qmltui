#include <QtTest>

#include <algorithm>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>

#include <TextGridBuffer.h>
#include <UnicodeText.h>

#include "MarkdownDocument.h"
#include "MarkdownPalette.h"
#include "MarkdownView.h"

using namespace qtui;

namespace {
QString renderedText(const MarkdownDocument &document) {
    QStringList lines;
    for (const DocumentLine &line : document.renderedLines()) {
        QString text;
        for (const FormattedSegment &segment : line.segments)
            text += segment.text;
        lines.append(text);
    }
    return lines.join(QLatin1Char('\n'));
}

int renderedWidth(const DocumentLine &line) {
    int width = line.indent + line.quoteDepth * 2;
    for (const FormattedSegment &segment : line.segments)
        width += UnicodeText::displayWidth(segment.text);
    return width;
}
}

class MarkdownViewerTest : public QObject {
    Q_OBJECT

private slots:
    void whitespaceSearchIsEmpty();
    void linkedHeadingBuildsMetadata();
    void nestedBlocksArePreserved();
    void wrappingIsUnicodeSafe();
    void relayoutPreservesMetadata();
    void reloadReadsTheSourceAgain();
    void viewTracksDocumentLifetimeAndClampsScroll();
    void renderingHonorsClipAndSearchStyle();
    void paletteResolvesSemanticRoles();
};

void MarkdownViewerTest::whitespaceSearchIsEmpty() {
    MarkdownDocument document;
    document.setText(QStringLiteral("alpha beta alpha"));

    QVERIFY(document.searchMatches(QStringLiteral("   \t\n")).isEmpty());
    QCOMPARE(document.search(QStringLiteral("   ")).size(), 0);
    QCOMPARE(document.searchMatches(QStringLiteral("alpha")).size(), 2);
}

void MarkdownViewerTest::linkedHeadingBuildsMetadata() {
    MarkdownDocument document;
    document.setText(QStringLiteral("# Linked *title* [site](https://example.com)\n"));

    QCOMPARE(document.documentTitle(), QStringLiteral("Linked title site"));
    QCOMPARE(document.headingCount(), 1);
    QCOMPARE(document.linkCount(), 1);
    QCOMPARE(document.tableOfContents().size(), 1);
    QCOMPARE(document.tableOfContents().first().toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("Linked title site"));
    QCOMPARE(document.links().first().toMap().value(QStringLiteral("url")).toString(),
             QStringLiteral("https://example.com"));
}

void MarkdownViewerTest::nestedBlocksArePreserved() {
    MarkdownDocument document;
    document.setWrapWidth(40);
    document.setText(QStringLiteral(
        "> quoted\n>\n> - nested item\n>\n> ```cpp\n> const int value = 42;\n> ```\n"));

    const QString text = renderedText(document);
    QVERIFY2(text.contains(QStringLiteral("quoted")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("nested item")), qPrintable(text));
    QVERIFY2(text.contains(QStringLiteral("const int value = 42;")), qPrintable(text));
    QVERIFY(std::any_of(document.renderedLines().cbegin(), document.renderedLines().cend(),
                        [](const DocumentLine &line) { return line.quoteDepth > 0; }));
    QCOMPARE(document.codeBlockCount(), 1);
}

void MarkdownViewerTest::wrappingIsUnicodeSafe() {
    MarkdownDocument document;
    document.setWrapWidth(18);
    document.setText(QString::fromUtf8(
        "# 東京 👩‍💻\n\n"
        "supercalifragilisticexpialidocious 東京 👩‍💻 end\n\n"
        "```cpp\nconst auto emoji = \"👩‍💻\";\n```\n"));

    for (const DocumentLine &line : document.renderedLines())
        QVERIFY2(renderedWidth(line) <= document.wrapWidth(),
                 qPrintable(QStringLiteral("line width %1: %2")
                                .arg(renderedWidth(line)).arg(renderedText(document))));
    const QString text = renderedText(document);
    QVERIFY(text.contains(QString::fromUtf8("東京")));
    QVERIFY(text.contains(QString::fromUtf8("👩‍💻")));

    document.setWrapWidth(1);
    document.setText(QString::fromUtf8("👩‍💻東京"));
    QVERIFY(!document.renderedLines().isEmpty());
    for (const DocumentLine &line : document.renderedLines())
        QVERIFY(renderedWidth(line) <= 1);
}

void MarkdownViewerTest::relayoutPreservesMetadata() {
    MarkdownDocument document;
    document.setText(QStringLiteral(
        "# Title\n\nA paragraph with enough words to wrap at a narrow width.\n\n"
        "[link](https://example.com)\n"));
    const int headings = document.headingCount();
    const int links = document.linkCount();
    const QString title = document.documentTitle();
    const int wideLines = document.lineCount();

    document.setWrapWidth(12);
    QVERIFY(document.lineCount() > wideLines);
    QCOMPARE(document.headingCount(), headings);
    QCOMPARE(document.linkCount(), links);
    QCOMPARE(document.documentTitle(), title);
}

void MarkdownViewerTest::reloadReadsTheSourceAgain() {
    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write("# First\n"), qint64(8));
    QVERIFY(file.flush());

    MarkdownDocument document;
    document.setSource(QUrl::fromLocalFile(file.fileName()).toString());
    QCOMPARE(document.documentTitle(), QStringLiteral("First"));

    QVERIFY(file.resize(0));
    QCOMPARE(file.write("# Second\n"), qint64(9));
    QVERIFY(file.flush());
    document.reload();
    QCOMPARE(document.documentTitle(), QStringLiteral("Second"));
}

void MarkdownViewerTest::viewTracksDocumentLifetimeAndClampsScroll() {
    MarkdownView view;
    view.setWidth(30);
    view.setHeight(5);
    auto *document = new MarkdownDocument;
    document->setText(QString(100, QLatin1Char('x')).replace(QLatin1Char('x'),
        QStringLiteral("line\n")));
    view.setDocument(document);
    const int gutterWidth = QString::number(document->lineCount()).size() + 3;
    QCOMPARE(document->wrapWidth(), view.width() - gutterWidth);

    view.setScrollY(10000);
    QVERIFY(view.scrollY() > 0);
    document->setText(QStringLiteral("short"));
    QCOMPARE(view.scrollY(), 0);

    delete document;
    QCOMPARE(view.document(), nullptr);
    QCOMPARE(view.contentHeight(), 0);
}

void MarkdownViewerTest::renderingHonorsClipAndSearchStyle() {
    MarkdownDocument document;
    document.setText(QStringLiteral("prefix match suffix"));

    MarkdownView view;
    view.setWidth(24);
    view.setHeight(5);
    view.setBorderStyle(QStringLiteral("single"));
    view.setDocument(&document);
    view.setSearchQuery(QStringLiteral("match"));

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(24, 5));
    buffer.fillRect(0, 0, 24, 5, ".");
    const ClipRect clip{4, 0, 10, 3};
    view.render(buffer, 0, 0, clip);

    bool changedInside = false;
    bool highlighted = false;
    const uint32_t activeBackground = view.palette().resolvedActiveMatchBg();
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            const Cell &cell = buffer.cellAt(x, y);
            if (clip.contains(x, y)) {
                changedInside |= cell.character != QByteArray(".");
                highlighted |= cell.bgColor == activeBackground;
            } else {
                QCOMPARE(cell.character, QByteArray("."));
            }
        }
    }
    QVERIFY(changedInside);
    QVERIFY(highlighted);

    TextGridBuffer fullBuffer;
    QVERIFY(fullBuffer.resize(24, 5));
    view.render(fullBuffer, 0, 0);
    QCOMPARE(fullBuffer.cellAt(1, 1).character, QByteArray("1"));
    QCOMPARE(fullBuffer.cellAt(3, 1).character, QStringLiteral("│").toUtf8());
}

void MarkdownViewerTest::paletteResolvesSemanticRoles() {
    MarkdownPalette palette;
    palette.m_link = QStringLiteral("#010203");
    palette.m_syntaxString = QStringLiteral("#a0b0c0");
    QCOMPARE(palette.resolve(MarkdownRole::Link), 0xff010203u);
    QCOMPARE(palette.resolve(MarkdownRole::SyntaxString), 0xffa0b0c0u);
    QCOMPARE(palette.resolve(MarkdownRole::Transparent), 0u);
}

QTEST_MAIN(MarkdownViewerTest)
#include "test_markdown_viewer.moc"
