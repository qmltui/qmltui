#include <QtTest>

#include <algorithm>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>

#include <TextGridBuffer.h>
#include <UnicodeText.h>
#include <ItemPaintTraversal_p.h>

#include "MdDoc.h"
#include "MdDocStyle.h"
#include "MdPalette.h"
#include "MdRole.h"
#include "MdView.h"
#include "MdViewStyle.h"
#include "SyntaxHighlighter.h"

using namespace qtui;

namespace {
QString renderedText(const MdDoc &document) {
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
    void codeBlockBackgroundIsConsistent();
    void docStylePropertiesAndOverrides();
    void modernH1BadgeAndHeadings();
    void haskellSyntaxHighlighting();
    void modernTableRendering();
    void showLineNumbersToggle();
};

void MarkdownViewerTest::whitespaceSearchIsEmpty() {
    MdDoc document;
    document.setText(QStringLiteral("alpha beta alpha"));

    QVERIFY(document.searchMatches(QStringLiteral("   \t\n")).isEmpty());
    QCOMPARE(document.search(QStringLiteral("   ")).size(), 0);
    QCOMPARE(document.searchMatches(QStringLiteral("alpha")).size(), 2);
}

void MarkdownViewerTest::linkedHeadingBuildsMetadata() {
    MdDoc document;
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
    MdDoc document;
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
    MdDoc document;
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
    MdDoc document;
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

    MdDoc document;
    document.setSource(QUrl::fromLocalFile(file.fileName()).toString());
    QCOMPARE(document.documentTitle(), QStringLiteral("First"));

    QVERIFY(file.resize(0));
    QCOMPARE(file.write("# Second\n"), qint64(9));
    QVERIFY(file.flush());
    document.reload();
    QCOMPARE(document.documentTitle(), QStringLiteral("Second"));
}

void MarkdownViewerTest::viewTracksDocumentLifetimeAndClampsScroll() {
    MdView view;
    view.setShowLineNumbers(true);
    view.setWidth(30);
    view.setHeight(5);
    auto *document = new MdDoc;
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
    MdDoc document;
    document.setText(QStringLiteral("prefix match suffix"));

    MdView view;
    view.setShowLineNumbers(true);
    view.setWidth(24);
    view.setHeight(5);
    view.setDocument(&document);
    view.setSearchQuery(QStringLiteral("match"));

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(24, 5));
    buffer.fillRect(0, 0, 24, 5, ".");
    const ClipRect clip{4, 0, 10, 3};
    ItemPaintTraversal::paintSubtree(&view, buffer, 0, 0, clip);

    bool changedInside = false;
    bool highlighted = false;
    const uint32_t activeBackground = view.palette().resolvedActiveMatchBg();
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            const Cell &cell = buffer.cellAt(x, y);
            if (clip.contains(x, y)) {
                changedInside |= cell.character().toByteArray() != QByteArray(".");
                highlighted |= cell.bgColor == activeBackground;
            } else {
                QCOMPARE(cell.character().toByteArray(), QByteArray("."));
            }
        }
    }
    QVERIFY(changedInside);
    QVERIFY(highlighted);

    TextGridBuffer fullBuffer;
    QVERIFY(fullBuffer.resize(24, 5));
    ItemPaintTraversal::paintSubtree(&view, fullBuffer, 0, 0);
    QCOMPARE(fullBuffer.cellAt(0, 0).character().toByteArray(), QByteArray("1"));
    QCOMPARE(fullBuffer.cellAt(2, 0).character().toByteArray(), QStringLiteral("│").toUtf8());
    QCOMPARE(view.metaObject()->indexOfProperty("borderStyle"), -1);
}

void MarkdownViewerTest::paletteResolvesSemanticRoles() {
    MdPalette palette;
    palette.m_link = QStringLiteral("#010203");
    palette.m_syntaxString = QStringLiteral("#a0b0c0");
    palette.m_heading1Bg = QStringLiteral("#5856d6");
    palette.m_inlineCode = QStringLiteral("#f43f5e");
    palette.m_inlineCodeBg = QStringLiteral("#4c1d24");
    palette.m_syntaxFunction = QStringLiteral("#50fa7b");

    QCOMPARE(palette.resolve(MdRole::Link), 0xff010203u);
    QCOMPARE(palette.resolve(MdRole::SyntaxString), 0xffa0b0c0u);
    QCOMPARE(palette.resolve(MdRole::Heading1Bg), 0xff5856d6u);
    QCOMPARE(palette.resolve(MdRole::InlineCode), 0xfff43f5eu);
    QCOMPARE(palette.resolve(MdRole::InlineCodeBackground), 0xff4c1d24u);
    QCOMPARE(palette.resolve(MdRole::SyntaxFunction), 0xff50fa7bu);
    QCOMPARE(palette.resolve(MdRole::Transparent), 0u);
}

void MarkdownViewerTest::codeBlockBackgroundIsConsistent() {
    MdDoc document;
    document.setWrapWidth(40);
    document.setText(QStringLiteral("```cpp\nint x = 42;\n```\n\nInline `code_badge` here.\n"));

    // Check fenced code block lines
    bool foundCodeBlockLine = false;
    for (const DocumentLine &line : document.renderedLines()) {
        if (line.isCodeBlock) {
            foundCodeBlockLine = true;
            for (const FormattedSegment &segment : line.segments) {
                QCOMPARE(segment.background, MdRole::Transparent);
            }
        }
    }
    QVERIFY(foundCodeBlockLine);

    // Check inline code span uses InlineCodeBackground
    bool foundInlineCode = false;
    for (const DocumentLine &line : document.renderedLines()) {
        if (!line.isCodeBlock) {
            for (const FormattedSegment &segment : line.segments) {
                if (segment.text.contains(QStringLiteral("code_badge"))) {
                    foundInlineCode = true;
                    QCOMPARE(segment.background, MdRole::InlineCodeBackground);
                }
            }
        }
    }
    QVERIFY(foundInlineCode);
}

void MarkdownViewerTest::docStylePropertiesAndOverrides() {
    MdDocStyle style;
    QCOMPARE(style.h1Format(), MdDocStyle::Heading1Format::Badge);
    QCOMPARE(style.tableFormat(), MdDocStyle::TableFormat::Open);
    QCOMPARE(style.codeBlockFormat(), MdDocStyle::CodeBlockFormat::Indented);

    style.setH1Format(MdDocStyle::Heading1Format::Banner);
    style.setTableFormat(MdDocStyle::TableFormat::Boxed);
    style.setCodeBlockFormat(MdDocStyle::CodeBlockFormat::Boxed);
    QCOMPARE(style.h1Format(), MdDocStyle::Heading1Format::Banner);
    QCOMPARE(style.tableFormat(), MdDocStyle::TableFormat::Boxed);
    QCOMPARE(style.codeBlockFormat(), MdDocStyle::CodeBlockFormat::Boxed);

    // Granular override
    style.setH1Format(MdDocStyle::Heading1Format::Badge);
    QCOMPARE(style.h1Format(), MdDocStyle::Heading1Format::Badge);
    QCOMPARE(style.tableFormat(), MdDocStyle::TableFormat::Boxed);

    MdDoc document;
    document.setStyle(&style);
    document.setText(QStringLiteral("# Title\n"));
    QVERIFY(!document.renderedLines().isEmpty());
    QCOMPARE(document.renderedLines().first().segments.first().background, MdRole::Heading1Bg);
}

void MarkdownViewerTest::modernH1BadgeAndHeadings() {
    MdDoc document;
    MdDocStyle style;
    style.setHeadingFormat(MdDocStyle::HeadingFormat::Hashes);
    document.setStyle(&style);
    document.setText(QStringLiteral("# Showcase\n\n## Section 2\n\n### Section 3\n"));

    QVERIFY(!document.renderedLines().isEmpty());
    const DocumentLine &h1Line = document.renderedLines().first();
    QVERIFY(h1Line.isHeading);
    QCOMPARE(h1Line.headingLevel, 1);

    // Check H1 has Heading1Bg background segments
    bool hasH1Bg = false;
    for (const FormattedSegment &seg : h1Line.segments) {
        if (seg.background == MdRole::Heading1Bg)
            hasH1Bg = true;
    }
    QVERIFY(hasH1Bg);

    // Check H2 has "## " prefix
    const QString text = renderedText(document);
    QVERIFY(text.contains(QStringLiteral("## Section 2")));
    QVERIFY(text.contains(QStringLiteral("### Section 3")));
}

void MarkdownViewerTest::haskellSyntaxHighlighting() {
    const QString haskellCode = QStringLiteral("module Main where\nimport Data.List (intercalate)\nhello :: String -> String\nmain = putStrLn $ intercalate \"\\n\" [ \"a\" ]\n");
    const auto lang = SyntaxHighlighter::detectLanguage(QStringLiteral("haskell"));
    QCOMPARE(lang, SyntaxHighlighter::Language::Haskell);

    const auto lines = haskellCode.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QVERIFY(lines.size() >= 3);

    // Line 1: module Main where
    const auto line1Spans = SyntaxHighlighter::highlightLine(lines[0], lang);
    bool foundModule = false;
    bool foundMain = false;
    for (const StyledSpan &span : line1Spans) {
        if (span.text == QStringLiteral("module") && span.foreground == MdRole::SyntaxKeyword)
            foundModule = true;
        if (span.text == QStringLiteral("Main") && span.foreground == MdRole::SyntaxType)
            foundMain = true;
    }
    QVERIFY(foundModule);
    QVERIFY(foundMain);

    // Line 3: hello :: String -> String
    const auto line3Spans = SyntaxHighlighter::highlightLine(lines[2], lang);
    bool foundDoubleColon = false;
    bool foundString = false;
    bool foundArrow = false;
    for (const StyledSpan &span : line3Spans) {
        if (span.text == QStringLiteral("::") && span.foreground == MdRole::SyntaxSymbol)
            foundDoubleColon = true;
        if (span.text == QStringLiteral("String") && span.foreground == MdRole::SyntaxType)
            foundString = true;
        if (span.text == QStringLiteral("->") && span.foreground == MdRole::SyntaxSymbol)
            foundArrow = true;
    }
    QVERIFY(foundDoubleColon);
    QVERIFY(foundString);
    QVERIFY(foundArrow);
}

void MarkdownViewerTest::modernTableRendering() {
    MdDoc document;
    MdDocStyle style;
    style.setTableFormat(MdDocStyle::TableFormat::Open);
    document.setStyle(&style);
    document.setWrapWidth(80);
    document.setText(QStringLiteral(
        "| Name | Power | Comment |\n"
        "| :--- | :--- | :--- |\n"
        "| Carrots | 9001 | It's over 9000?! |\n"
        "| Ramen | 9002 | Also over 9000?! |\n"));

    const QString text = renderedText(document);
    QVERIFY(text.contains(QStringLiteral("Name")));
    QVERIFY(text.contains(QStringLiteral("Power")));
    QVERIFY(text.contains(QStringLiteral("Carrots")));
    QVERIFY(text.contains(QStringLiteral("9001")));
    QVERIFY(text.contains(QStringLiteral("+")));
}

void MarkdownViewerTest::showLineNumbersToggle() {
    MdView view;
    view.setWidth(80);
    view.setHeight(24);
    MdDoc document;
    document.setText(QStringLiteral("Line 1\nLine 2\nLine 3\n"));
    view.setDocument(&document);

    view.setShowLineNumbers(false);
    QCOMPARE(view.showLineNumbers(), false);
    QCOMPARE(document.wrapWidth(), 80);

    view.setShowLineNumbers(true);
    QCOMPARE(view.showLineNumbers(), true);
    QVERIFY(document.wrapWidth() < 80);
}

QTEST_MAIN(MarkdownViewerTest)
#include "test_markdown_viewer.moc"
