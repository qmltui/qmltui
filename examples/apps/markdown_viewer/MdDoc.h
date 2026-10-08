#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QList>
#include <QSharedPointer>
#include <QPointer>
#include <QtQml/qqmlregistration.h>
#include <Qtui/Core/primitives/Cell.h>

#include "MdRole.h"
#include "MdDocStyle.h"
#include "SyntaxHighlighter.h"

namespace MD {
class Document;
class Item;
class Block;
class Heading;
class Paragraph;
class Code;
class Blockquote;
class List;
class ListItem;
class Table;
}

namespace qtui {

struct FormattedSegment {
    QString text;
    MdRole foreground = MdRole::Text;
    MdRole background = MdRole::Transparent;
    TextAttribute attr = TextAttribute::None;
    QString href;
    QString linkLabel;
    int linkId = -1;
};

struct DocumentLine {
    QList<FormattedSegment> segments;
    int indent = 0;
    bool isHeading = false;
    int headingLevel = 0;
    bool isCodeBlock = false;
    bool isTable = false;
    int quoteDepth = 0;
    bool isHorizontalRule = false;
};

struct TocItem {
    int level = 1;
    QString title;
    int lineIndex = 0;
};

struct LinkEntry {
    QString text;
    QString url;
    int lineIndex = 0;
};

struct SearchMatch {
    int line = 0;
    int utf16Column = 0;
    int utf16Length = 0;

    bool operator==(const SearchMatch &) const = default;
};

class MdDoc : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString documentTitle READ documentTitle NOTIFY parsedChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY parsedChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY parsedChanged)
    Q_PROPERTY(int headingCount READ headingCount NOTIFY parsedChanged)
    Q_PROPERTY(int codeBlockCount READ codeBlockCount NOTIFY parsedChanged)
    Q_PROPERTY(int tableCount READ tableCount NOTIFY parsedChanged)
    Q_PROPERTY(int linkCount READ linkCount NOTIFY parsedChanged)
    Q_PROPERTY(QVariantList tableOfContents READ tableOfContents NOTIFY parsedChanged)
    Q_PROPERTY(QVariantList links READ links NOTIFY parsedChanged)
    Q_PROPERTY(int wrapWidth READ wrapWidth WRITE setWrapWidth NOTIFY wrapWidthChanged)
    Q_PROPERTY(qtui::MdDocStyle* style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(QString renderStyle READ renderStyle WRITE setRenderStyle NOTIFY renderStyleChanged)
    Q_PROPERTY(QString codeBlockStyle READ codeBlockStyle WRITE setCodeBlockStyle NOTIFY codeBlockStyleChanged)
    Q_PROPERTY(QString tableStyle READ tableStyle WRITE setTableStyle NOTIFY tableStyleChanged)

public:
    explicit MdDoc(QObject *parent = nullptr);
    ~MdDoc() override = default;

    QString source() const { return m_source; }
    void setSource(const QString &source);

    QString text() const { return m_text; }
    void setText(const QString &text);

    int wrapWidth() const { return m_wrapWidth; }
    void setWrapWidth(int width);

    MdDocStyle* style() const;
    void setStyle(MdDocStyle *style);

    QString renderStyle() const;
    void setRenderStyle(const QString &style);

    QString codeBlockStyle() const { return m_codeBlockStyle; }
    void setCodeBlockStyle(const QString &style);

    QString tableStyle() const { return m_tableStyle; }
    void setTableStyle(const QString &style);

    QString documentTitle() const { return m_documentTitle; }
    int lineCount() const { return m_renderedLines.size(); }
    int wordCount() const { return m_wordCount; }
    int headingCount() const { return m_headingCount; }
    int codeBlockCount() const { return m_codeBlockCount; }
    int tableCount() const { return m_tableCount; }
    int linkCount() const { return m_links.size(); }

    QVariantList tableOfContents() const;
    QVariantList links() const;

    const QList<DocumentLine> &renderedLines() const { return m_renderedLines; }
    QList<SearchMatch> searchMatches(const QString &query) const;

    MdDocStyle *effectiveStyle() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QVariantList search(const QString &query) const;
    Q_INVOKABLE QString rawContent() const { return m_text; }

signals:
    void sourceChanged();
    void textChanged();
    void wrapWidthChanged();
    void styleChanged();
    void renderStyleChanged();
    void codeBlockStyleChanged();
    void tableStyleChanged();
    void parsedChanged();

private:
    bool loadSource();
    void parseDocument();
    void collectMetadata(const QSharedPointer<MD::Item> &item);
    void layoutDocument();
    void processItem(const QSharedPointer<MD::Item> &item, int indent = 0,
                     int quoteDepth = 0);
    void processHeading(const QSharedPointer<MD::Heading> &heading, int indent,
                        int quoteDepth);
    void processParagraph(const QSharedPointer<MD::Paragraph> &paragraph, int indent,
                          int quoteDepth, const QList<FormattedSegment> &prefix = {});
    void processCode(const QSharedPointer<MD::Code> &code, int indent, int quoteDepth);
    void processBlockquote(const QSharedPointer<MD::Blockquote> &blockquote, int indent,
                           int quoteDepth);
    void processList(const QSharedPointer<MD::List> &list, int indent, int quoteDepth);
    void processListItem(const QSharedPointer<MD::ListItem> &listItem, int index,
                         bool ordered, int indent, int quoteDepth);
    void processTable(const QSharedPointer<MD::Table> &table, int indent,
                      int quoteDepth);

    QList<FormattedSegment> extractInlineSegments(
        const QSharedPointer<MD::Paragraph> &paragraph,
        MdRole defaultRole = MdRole::Text);
    QList<FormattedSegment> extractBlockSegments(
        const QSharedPointer<MD::Block> &block,
        MdRole defaultRole = MdRole::Text);
    static QString truncateToDisplayWidth(const QString &text, int width);
    static int segmentsWidth(const QList<FormattedSegment> &segments);
    void appendWrappedLine(const QList<FormattedSegment> &segments, int indent,
                           int quoteDepth);
    static QString plainText(const QSharedPointer<MD::Item> &item);
    void rebuildLinks();

    QString m_source;
    QString m_text;
    int m_wrapWidth = 80;
    QPointer<MdDocStyle> m_style;
    MdDocStyle m_fallbackStyle;
    QString m_codeBlockStyle = QStringLiteral("auto");
    QString m_tableStyle = QStringLiteral("auto");

    QSharedPointer<MD::Document> m_parsedDocument;
    QList<DocumentLine> m_renderedLines;
    QList<TocItem> m_toc;
    QList<LinkEntry> m_links;
    int m_nextLinkId = 1;

    QString m_documentTitle = QStringLiteral("Untitled Document");
    int m_wordCount = 0;
    int m_headingCount = 0;
    int m_codeBlockCount = 0;
    int m_tableCount = 0;
};

} // namespace qtui
