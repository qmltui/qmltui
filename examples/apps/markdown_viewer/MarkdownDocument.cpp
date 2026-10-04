#include "MarkdownDocument.h"

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QUrl>
#include <algorithm>

#include <UnicodeText.h>

#include "doc.h"
#include "parser.h"

namespace qtui {
namespace {

MarkdownRole headingRole(int level) {
    switch (std::clamp(level, 1, 6)) {
    case 1: return MarkdownRole::Heading1;
    case 2: return MarkdownRole::Heading2;
    case 3: return MarkdownRole::Heading3;
    case 4: return MarkdownRole::Heading4;
    case 5: return MarkdownRole::Heading5;
    default: return MarkdownRole::Heading6;
    }
}

QString headingPrefix(int level) {
    switch (std::clamp(level, 1, 6)) {
    case 1: return QStringLiteral("■ ");
    case 2: return QStringLiteral("● ");
    case 3: return QStringLiteral("◆ ");
    case 4: return QStringLiteral("◇ ");
    default: return QStringLiteral("▪ ");
    }
}

} // namespace

MarkdownDocument::MarkdownDocument(QObject *parent) : QObject(parent) {}

void MarkdownDocument::setSource(const QString &source) {
    if (m_source == source)
        return;
    m_source = source;
    emit sourceChanged();
    loadSource();
}

bool MarkdownDocument::loadSource() {
    QString filePath = m_source;
    const QUrl url(m_source);
    if (url.isLocalFile())
        filePath = url.toLocalFile();
    else if (filePath.startsWith(QStringLiteral("qrc:/")))
        filePath = QStringLiteral(":") + filePath.mid(4);

    QFile file(filePath);
    const bool opened = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (opened) {
        QTextStream stream(&file);
        m_text = stream.readAll();
    } else {
        m_text = QStringLiteral("# Error\n\nCould not open file: `%1`").arg(filePath);
    }
    emit textChanged();
    parseDocument();
    return opened;
}

void MarkdownDocument::setText(const QString &text) {
    if (m_text == text)
        return;
    m_text = text;
    emit textChanged();
    parseDocument();
}

void MarkdownDocument::setWrapWidth(int width) {
    const int clamped = std::max(1, width);
    if (m_wrapWidth == clamped)
        return;
    m_wrapWidth = clamped;
    emit wrapWidthChanged();
    layoutDocument();
}

void MarkdownDocument::reload() {
    if (!m_source.isEmpty())
        loadSource();
    else
        parseDocument();
}

QVariantList MarkdownDocument::tableOfContents() const {
    QVariantList result;
    result.reserve(m_toc.size());
    for (const TocItem &item : m_toc) {
        result.append(QVariantMap{{QStringLiteral("level"), item.level},
                                  {QStringLiteral("title"), item.title},
                                  {QStringLiteral("line"), item.lineIndex}});
    }
    return result;
}

QVariantList MarkdownDocument::links() const {
    QVariantList result;
    result.reserve(m_links.size());
    for (const LinkEntry &link : m_links) {
        result.append(QVariantMap{{QStringLiteral("text"), link.text},
                                  {QStringLiteral("url"), link.url},
                                  {QStringLiteral("line"), link.lineIndex}});
    }
    return result;
}

QList<SearchMatch> MarkdownDocument::searchMatches(const QString &query) const {
    QList<SearchMatch> matches;
    const QString cleanQuery = query.trimmed();
    if (cleanQuery.isEmpty())
        return matches;
    const int queryLength = static_cast<int>(cleanQuery.size());

    for (int lineIndex = 0; lineIndex < m_renderedLines.size(); ++lineIndex) {
        QString lineText;
        for (const FormattedSegment &segment : m_renderedLines[lineIndex].segments)
            lineText += segment.text;

        int column = lineText.indexOf(cleanQuery, 0, Qt::CaseInsensitive);
        while (column >= 0) {
            matches.append({lineIndex, column, queryLength});
            column = lineText.indexOf(cleanQuery, column + queryLength,
                                      Qt::CaseInsensitive);
        }
    }
    return matches;
}

QVariantList MarkdownDocument::search(const QString &query) const {
    QVariantList result;
    for (const SearchMatch &match : searchMatches(query)) {
        QString lineText;
        for (const FormattedSegment &segment : m_renderedLines[match.line].segments)
            lineText += segment.text;
        result.append(QVariantMap{{QStringLiteral("line"), match.line},
                                  {QStringLiteral("column"), match.utf16Column},
                                  {QStringLiteral("length"), match.utf16Length},
                                  {QStringLiteral("text"), lineText.trimmed()}});
    }
    return result;
}

void MarkdownDocument::parseDocument() {
    m_wordCount = m_text.split(QRegularExpression(QStringLiteral("\\s+")),
                               Qt::SkipEmptyParts).size();
    m_headingCount = 0;
    m_codeBlockCount = 0;
    m_tableCount = 0;
    m_documentTitle = QStringLiteral("Markdown Document");

    m_parsedDocument.reset();
    if (!m_text.isEmpty()) {
        MD::Parser parser;
        QTextStream stream(&m_text);
        m_parsedDocument = parser.parse(stream, QString(), QString());
    }

    if (m_parsedDocument)
        collectMetadata(qSharedPointerCast<MD::Item>(m_parsedDocument));
    layoutDocument();
}

void MarkdownDocument::collectMetadata(const QSharedPointer<MD::Item> &item) {
    if (!item)
        return;

    switch (item->type()) {
    case MD::ItemType::Heading: {
        const auto heading = qSharedPointerCast<MD::Heading>(item);
        ++m_headingCount;
        const QString title = plainText(qSharedPointerCast<MD::Item>(heading->text())).trimmed();
        if (m_documentTitle == QStringLiteral("Markdown Document") && heading->level() == 1
            && !title.isEmpty()) {
            m_documentTitle = title;
        }
        collectMetadata(qSharedPointerCast<MD::Item>(heading->text()));
        return;
    }
    case MD::ItemType::Code: {
        const auto code = qSharedPointerCast<MD::Code>(item);
        if (!code->isInline())
            ++m_codeBlockCount;
        return;
    }
    case MD::ItemType::Table: {
        const auto table = qSharedPointerCast<MD::Table>(item);
        ++m_tableCount;
        for (const auto &row : table->rows()) {
            if (!row)
                continue;
            for (const auto &cell : row->cells())
                collectMetadata(qSharedPointerCast<MD::Item>(cell));
        }
        return;
    }
    case MD::ItemType::Paragraph:
    case MD::ItemType::Blockquote:
    case MD::ItemType::ListItem:
    case MD::ItemType::List:
    case MD::ItemType::TableCell:
    case MD::ItemType::Document: {
        const auto block = qSharedPointerCast<MD::Block>(item);
        for (const auto &child : block->items())
            collectMetadata(child);
        return;
    }
    case MD::ItemType::Link:
    case MD::ItemType::Image: {
        const auto link = qSharedPointerCast<MD::LinkBase>(item);
        if (link->p())
            collectMetadata(qSharedPointerCast<MD::Item>(link->p()));
        return;
    }
    default:
        return;
    }
}

QString MarkdownDocument::plainText(const QSharedPointer<MD::Item> &item) const {
    if (!item)
        return {};

    switch (item->type()) {
    case MD::ItemType::Text:
        return qSharedPointerCast<MD::Text>(item)->text();
    case MD::ItemType::Code:
        return qSharedPointerCast<MD::Code>(item)->text();
    case MD::ItemType::LineBreak:
        return QStringLiteral("\n");
    case MD::ItemType::Link:
    case MD::ItemType::Image: {
        const auto link = qSharedPointerCast<MD::LinkBase>(item);
        const QString parsed = plainText(qSharedPointerCast<MD::Item>(link->p()));
        return parsed.isEmpty() ? link->text() : parsed;
    }
    case MD::ItemType::Heading:
        return plainText(qSharedPointerCast<MD::Item>(
            qSharedPointerCast<MD::Heading>(item)->text()));
    case MD::ItemType::Paragraph:
    case MD::ItemType::Blockquote:
    case MD::ItemType::ListItem:
    case MD::ItemType::List:
    case MD::ItemType::TableCell:
    case MD::ItemType::Document: {
        QString text;
        for (const auto &child : qSharedPointerCast<MD::Block>(item)->items())
            text += plainText(child);
        return text;
    }
    case MD::ItemType::Table: {
        QString text;
        const auto table = qSharedPointerCast<MD::Table>(item);
        for (const auto &row : table->rows()) {
            if (!row)
                continue;
            for (const auto &cell : row->cells())
                text += plainText(qSharedPointerCast<MD::Item>(cell));
        }
        return text;
    }
    default:
        return {};
    }
}

void MarkdownDocument::layoutDocument() {
    m_toc.clear();
    m_links.clear();
    m_renderedLines.clear();
    m_nextLinkId = 0;

    if (m_parsedDocument) {
        for (const auto &item : m_parsedDocument->items())
            processItem(item);
    }
    rebuildLinks();
    emit parsedChanged();
}

void MarkdownDocument::processItem(const QSharedPointer<MD::Item> &item, int indent,
                                   int quoteDepth) {
    if (!item)
        return;

    switch (item->type()) {
    case MD::ItemType::Heading:
        processHeading(qSharedPointerCast<MD::Heading>(item), indent, quoteDepth);
        break;
    case MD::ItemType::Paragraph:
        processParagraph(qSharedPointerCast<MD::Paragraph>(item), indent, quoteDepth);
        break;
    case MD::ItemType::Code: {
        const auto code = qSharedPointerCast<MD::Code>(item);
        if (code->isInline()) {
            appendWrappedLine({{code->text(), MarkdownRole::Code,
                                MarkdownRole::CodeBackground, TextAttribute::Bold}},
                              indent, quoteDepth);
        } else {
            processCode(code, indent, quoteDepth);
        }
        break;
    }
    case MD::ItemType::Blockquote:
        processBlockquote(qSharedPointerCast<MD::Blockquote>(item), indent, quoteDepth);
        break;
    case MD::ItemType::List:
        processList(qSharedPointerCast<MD::List>(item), indent, quoteDepth);
        break;
    case MD::ItemType::ListItem:
        processListItem(qSharedPointerCast<MD::ListItem>(item), 1, false, indent,
                        quoteDepth);
        break;
    case MD::ItemType::Table:
        processTable(qSharedPointerCast<MD::Table>(item), indent, quoteDepth);
        break;
    case MD::ItemType::HorizontalLine: {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isHorizontalRule = true;
        const int width = std::max(1, m_wrapWidth - indent - quoteDepth * 2);
        line.segments.append({QString(width, QChar(0x2500)), MarkdownRole::Border});
        m_renderedLines.append(line);
        m_renderedLines.append(DocumentLine{});
        break;
    }
    default: {
        const QString fallback = plainText(item).trimmed();
        if (!fallback.isEmpty())
            appendWrappedLine({{fallback, MarkdownRole::Text}}, indent, quoteDepth);
        break;
    }
    }
}

QList<FormattedSegment> MarkdownDocument::extractInlineSegments(
    const QSharedPointer<MD::Paragraph> &paragraph, MarkdownRole defaultRole) {
    return extractBlockSegments(qSharedPointerCast<MD::Block>(paragraph), defaultRole);
}

QList<FormattedSegment> MarkdownDocument::extractBlockSegments(
    const QSharedPointer<MD::Block> &block, MarkdownRole defaultRole) {
    QList<FormattedSegment> segments;
    if (!block)
        return segments;

    for (const auto &item : block->items()) {
        if (!item)
            continue;
        switch (item->type()) {
        case MD::ItemType::Text: {
            const auto text = qSharedPointerCast<MD::Text>(item);
            TextAttribute attributes = TextAttribute::None;
            if (text->opts() & MD::BoldText)
                attributes = attributes | TextAttribute::Bold;
            if (text->opts() & MD::ItalicText)
                attributes = attributes | TextAttribute::Italic;
            if (text->opts() & MD::StrikethroughText)
                attributes = attributes | TextAttribute::Strikethrough;
            segments.append({text->text(), defaultRole, MarkdownRole::Transparent,
                             attributes});
            break;
        }
        case MD::ItemType::Code: {
            const auto code = qSharedPointerCast<MD::Code>(item);
            segments.append({code->text(), MarkdownRole::Code,
                             MarkdownRole::CodeBackground, TextAttribute::Bold});
            break;
        }
        case MD::ItemType::Link:
        case MD::ItemType::Image: {
            const auto link = qSharedPointerCast<MD::LinkBase>(item);
            QString label = plainText(qSharedPointerCast<MD::Item>(link->p())).trimmed();
            if (label.isEmpty())
                label = link->text().trimmed();
            if (label.isEmpty())
                label = link->url();
            if (item->type() == MD::ItemType::Image)
                label = QStringLiteral("[Image: %1]").arg(label);
            const int linkId = m_nextLinkId++;
            segments.append({label,
                             item->type() == MD::ItemType::Image
                                 ? MarkdownRole::Success : MarkdownRole::Link,
                             MarkdownRole::Transparent, TextAttribute::Underline,
                             link->url(), label, linkId});
            break;
        }
        case MD::ItemType::Paragraph:
        case MD::ItemType::TableCell:
            segments.append(extractBlockSegments(qSharedPointerCast<MD::Block>(item),
                                                 defaultRole));
            break;
        case MD::ItemType::LineBreak:
            segments.append({QStringLiteral("\n"), defaultRole});
            break;
        default: {
            const QString fallback = plainText(item);
            if (!fallback.isEmpty())
                segments.append({fallback, defaultRole});
            break;
        }
        }
    }
    return segments;
}

QString MarkdownDocument::truncateToDisplayWidth(const QString &text, int width) {
    if (width <= 0)
        return {};
    QString result;
    int used = 0;
    for (const Grapheme &cluster : UnicodeText::graphemes(text)) {
        if (used + cluster.width > width)
            break;
        result += QString::fromUtf8(cluster.utf8);
        used += cluster.width;
    }
    return result;
}

int MarkdownDocument::segmentsWidth(const QList<FormattedSegment> &segments) {
    int width = 0;
    for (const FormattedSegment &segment : segments)
        width += UnicodeText::displayWidth(segment.text);
    return width;
}

void MarkdownDocument::appendWrappedLine(const QList<FormattedSegment> &segments,
                                         int indent, int quoteDepth) {
    const int maxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * 2);
    DocumentLine current;
    current.indent = indent;
    current.quoteDepth = quoteDepth;
    int usedWidth = 0;

    auto pushLine = [&](bool force = false) {
        if (!current.segments.isEmpty() || force) {
            m_renderedLines.append(current);
            current = DocumentLine{};
            current.indent = indent;
            current.quoteDepth = quoteDepth;
            usedWidth = 0;
        }
    };

    auto appendPiece = [&](const QString &piece, const FormattedSegment &style) {
        if (piece.isEmpty())
            return;
        QString pending = piece;
        while (!pending.isEmpty()) {
            const int remaining = maxWidth - usedWidth;
            QString portion = truncateToDisplayWidth(pending, remaining);
            if (portion.isEmpty()) {
                if (usedWidth > 0) {
                    pushLine();
                    continue;
                }

                // A double-width grapheme cannot fit in a one-cell viewport.
                // Consume it and render a one-cell replacement so layout always
                // makes progress without splitting the grapheme or overflowing.
                const QVector<Grapheme> clusters = UnicodeText::graphemes(pending);
                if (clusters.isEmpty())
                    break;
                FormattedSegment replacement = style;
                replacement.text = QStringLiteral("�");
                current.segments.append(replacement);
                usedWidth = 1;
                pending.remove(0, clusters.constFirst().utf16Length);
                if (!pending.isEmpty())
                    pushLine();
                continue;
            }
            FormattedSegment part = style;
            part.text = portion;
            current.segments.append(part);
            usedWidth += UnicodeText::displayWidth(portion);
            pending.remove(0, portion.size());
            if (!pending.isEmpty())
                pushLine();
        }
    };

    for (const FormattedSegment &segment : segments) {
        QString token;
        auto flushToken = [&]() {
            if (token.isEmpty())
                return;
            const int tokenWidth = UnicodeText::displayWidth(token);
            if (usedWidth > 0 && usedWidth + tokenWidth > maxWidth)
                pushLine();
            appendPiece(token, segment);
            token.clear();
        };

        for (const Grapheme &cluster : UnicodeText::graphemes(segment.text)) {
            const QString grapheme = QString::fromUtf8(cluster.utf8);
            if (grapheme == QStringLiteral("\n")) {
                flushToken();
                pushLine(true);
            } else if (grapheme == QStringLiteral(" ") || grapheme == QStringLiteral("\t")) {
                flushToken();
                if (usedWidth >= maxWidth)
                    pushLine();
                FormattedSegment space = segment;
                space.text = QStringLiteral(" ");
                current.segments.append(space);
                ++usedWidth;
            } else {
                token += grapheme;
            }
        }
        flushToken();
    }
    pushLine();
}

void MarkdownDocument::processHeading(const QSharedPointer<MD::Heading> &heading,
                                      int indent, int quoteDepth) {
    if (!heading || !heading->text())
        return;
    if (!m_renderedLines.isEmpty())
        m_renderedLines.append(DocumentLine{});

    const int level = std::clamp(heading->level(), 1, 6);
    const QString title = plainText(qSharedPointerCast<MD::Item>(heading->text())).trimmed();
    const int headingLine = m_renderedLines.size();
    m_toc.append({level, title, headingLine});

    const MarkdownRole role = headingRole(level);
    QList<FormattedSegment> segments{{headingPrefix(level), role,
                                      MarkdownRole::Transparent, TextAttribute::Bold}};
    QList<FormattedSegment> content = extractInlineSegments(heading->text(), role);
    for (FormattedSegment &segment : content) {
        segment.foreground = role;
        segment.attr = segment.attr | TextAttribute::Bold;
    }
    segments.append(content);
    const int firstLine = m_renderedLines.size();
    appendWrappedLine(segments, indent, quoteDepth);
    for (int i = firstLine; i < m_renderedLines.size(); ++i) {
        m_renderedLines[i].isHeading = true;
        m_renderedLines[i].headingLevel = level;
    }

    if (level <= 2) {
        DocumentLine banner;
        banner.indent = indent;
        banner.quoteDepth = quoteDepth;
        banner.isHeading = true;
        banner.headingLevel = level;
        const int available = std::max(1, m_wrapWidth - indent - quoteDepth * 2);
        const int desired = UnicodeText::displayWidth(title) + 4;
        const int length = std::min(available, std::max(level == 1 ? 20 : 15, desired));
        banner.segments.append({QString(length, level == 1 ? QChar(0x2550) : QChar(0x2500)),
                                role});
        m_renderedLines.append(banner);
    }
    m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::processParagraph(const QSharedPointer<MD::Paragraph> &paragraph,
                                        int indent, int quoteDepth,
                                        const QList<FormattedSegment> &prefix) {
    if (!paragraph)
        return;
    QList<FormattedSegment> content = prefix;
    content.append(extractInlineSegments(paragraph));
    appendWrappedLine(content, indent, quoteDepth);
    m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::processCode(const QSharedPointer<MD::Code> &code, int indent,
                                   int quoteDepth) {
    if (!code)
        return;
    const int boxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * 2);
    if (boxWidth < 4) {
        appendWrappedLine({{code->text(), MarkdownRole::Code,
                            MarkdownRole::CodeBackground}}, indent, quoteDepth);
        return;
    }

    const QString syntax = code->syntax().trimmed();
    const auto language = SyntaxHighlighter::detectLanguage(syntax);
    const int innerWidth = boxWidth - 2;
    QString label = QStringLiteral("─ [ %1 ] ").arg(
        syntax.isEmpty() ? QStringLiteral("code") : syntax.toLower());
    label = truncateToDisplayWidth(label, innerWidth);
    const int dashes = std::max(0, innerWidth - UnicodeText::displayWidth(label));

    DocumentLine top;
    top.indent = indent;
    top.quoteDepth = quoteDepth;
    top.isCodeBlock = true;
    top.segments.append({QStringLiteral("┌") + label
                             + QString(dashes, QChar(0x2500)) + QStringLiteral("┐"),
                         MarkdownRole::Border});
    m_renderedLines.append(top);

    const int contentWidth = boxWidth - 3;
    QStringList lines = code->text().split(QChar('\n'));
    if (!lines.isEmpty() && lines.constLast().isEmpty())
        lines.removeLast();
    for (const QString &sourceLine : std::as_const(lines)) {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isCodeBlock = true;
        line.segments.append({QStringLiteral("│ "), MarkdownRole::Border});
        int used = 0;
        for (const StyledSpan &span : SyntaxHighlighter::highlightLine(sourceLine, language)) {
            const QString text = truncateToDisplayWidth(span.text, contentWidth - used);
            if (text.isEmpty())
                break;
            line.segments.append({text, span.foreground, span.background, span.attributes});
            used += UnicodeText::displayWidth(text);
            if (text.size() != span.text.size())
                break;
        }
        if (used < contentWidth)
            line.segments.append({QString(contentWidth - used, QChar(' ')), MarkdownRole::Code,
                                  MarkdownRole::CodeBackground});
        line.segments.append({QStringLiteral("│"), MarkdownRole::Border});
        m_renderedLines.append(line);
    }

    DocumentLine bottom;
    bottom.indent = indent;
    bottom.quoteDepth = quoteDepth;
    bottom.isCodeBlock = true;
    bottom.segments.append({QStringLiteral("└") + QString(boxWidth - 2, QChar(0x2500))
                                + QStringLiteral("┘"),
                            MarkdownRole::Border});
    m_renderedLines.append(bottom);
    m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::processBlockquote(
    const QSharedPointer<MD::Blockquote> &blockquote, int indent, int quoteDepth) {
    if (!blockquote)
        return;
    for (const auto &item : blockquote->items())
        processItem(item, indent, quoteDepth + 1);
    if (!m_renderedLines.isEmpty() && !m_renderedLines.constLast().segments.isEmpty())
        m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::processList(const QSharedPointer<MD::List> &list, int indent,
                                   int quoteDepth) {
    if (!list)
        return;
    int fallbackNumber = 1;
    for (const auto &item : list->items()) {
        if (!item || item->type() != MD::ItemType::ListItem)
            continue;
        const auto listItem = qSharedPointerCast<MD::ListItem>(item);
        const bool ordered = listItem->listType() == MD::ListItem::Ordered;
        processListItem(listItem, ordered ? listItem->startNumber() : fallbackNumber,
                        ordered, indent, quoteDepth);
        ++fallbackNumber;
    }
    m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::processListItem(const QSharedPointer<MD::ListItem> &listItem,
                                       int index, bool ordered, int indent,
                                       int quoteDepth) {
    if (!listItem)
        return;
    QString marker;
    MarkdownRole markerRole = MarkdownRole::Link;
    if (listItem->isTaskList()) {
        marker = listItem->isChecked() ? QStringLiteral("[✓] ") : QStringLiteral("[ ] ");
        markerRole = listItem->isChecked() ? MarkdownRole::Success : MarkdownRole::Warning;
    } else if (ordered) {
        marker = QStringLiteral("%1. ").arg(index);
        markerRole = MarkdownRole::Warning;
    } else {
        marker = indent == 0 ? QStringLiteral("• ")
                             : (indent == 2 ? QStringLiteral("▪ ") : QStringLiteral("▫ "));
    }

    bool markerUsed = false;
    const int childIndent = indent + UnicodeText::displayWidth(marker);
    for (const auto &child : listItem->items()) {
        if (!child)
            continue;
        if (!markerUsed && child->type() == MD::ItemType::Paragraph) {
            processParagraph(qSharedPointerCast<MD::Paragraph>(child), indent, quoteDepth,
                             {{marker, markerRole, MarkdownRole::Transparent,
                               TextAttribute::Bold}});
            markerUsed = true;
        } else {
            if (!markerUsed) {
                appendWrappedLine({{marker, markerRole, MarkdownRole::Transparent,
                                    TextAttribute::Bold}}, indent, quoteDepth);
                markerUsed = true;
            }
            processItem(child, childIndent, quoteDepth);
        }
    }
    if (!markerUsed)
        appendWrappedLine({{marker, markerRole}}, indent, quoteDepth);
}

void MarkdownDocument::processTable(const QSharedPointer<MD::Table> &table, int indent,
                                    int quoteDepth) {
    if (!table || table->rows().isEmpty() || table->columnsCount() <= 0)
        return;
    const int columns = table->columnsCount();
    const int maxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * 2);
    QVector<QVector<QList<FormattedSegment>>> data;
    QVector<int> widths(columns, 3);

    for (int rowIndex = 0; rowIndex < table->rows().size(); ++rowIndex) {
        const auto &row = table->rows()[rowIndex];
        if (!row)
            continue;
        QVector<QList<FormattedSegment>> cells(columns);
        for (int column = 0; column < std::min(columns, int(row->cells().size())); ++column) {
            if (!row->cells()[column])
                continue;
            cells[column] = extractBlockSegments(row->cells()[column],
                                                  rowIndex == 0 ? MarkdownRole::Warning
                                                                : MarkdownRole::Text);
            if (rowIndex == 0) {
                for (FormattedSegment &segment : cells[column])
                    segment.attr = segment.attr | TextAttribute::Bold;
            }
            widths[column] = std::max(widths[column], segmentsWidth(cells[column]) + 2);
        }
        data.append(cells);
    }
    if (data.isEmpty())
        return;

    if (maxWidth < columns * 4 + 1) {
        for (const auto &row : std::as_const(data)) {
            QList<FormattedSegment> flattened;
            for (int column = 0; column < columns; ++column) {
                if (column)
                    flattened.append({QStringLiteral(" | "), MarkdownRole::Border});
                flattened.append(row[column]);
            }
            appendWrappedLine(flattened, indent, quoteDepth);
        }
        m_renderedLines.append(DocumentLine{});
        return;
    }

    auto totalWidth = [&]() {
        int total = 1;
        for (int width : std::as_const(widths))
            total += width + 1;
        return total;
    };
    while (totalWidth() > maxWidth) {
        const int widest = int(std::max_element(widths.cbegin(), widths.cend())
                               - widths.cbegin());
        if (widths[widest] <= 3)
            break;
        --widths[widest];
    }

    auto borderLine = [&](QChar left, QChar join, QChar right, QChar fill,
                          bool bold = false) {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isTable = true;
        QString text(left);
        for (int column = 0; column < columns; ++column) {
            text += QString(widths[column], fill);
            text += column == columns - 1 ? right : join;
        }
        line.segments.append({text, MarkdownRole::Border, MarkdownRole::Transparent,
                              bold ? TextAttribute::Bold : TextAttribute::None});
        m_renderedLines.append(line);
    };
    borderLine(QChar(0x250c), QChar(0x252c), QChar(0x2510), QChar(0x2500));

    for (int rowIndex = 0; rowIndex < data.size(); ++rowIndex) {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isTable = true;
        line.segments.append({QStringLiteral("│"), MarkdownRole::Border});
        for (int column = 0; column < columns; ++column) {
            const int contentWidth = widths[column] - 2;
            QList<FormattedSegment> clipped;
            int used = 0;
            for (const FormattedSegment &segment : data[rowIndex][column]) {
                const QString text = truncateToDisplayWidth(segment.text, contentWidth - used);
                if (text.isEmpty())
                    break;
                FormattedSegment copy = segment;
                copy.text = text;
                clipped.append(copy);
                used += UnicodeText::displayWidth(text);
                if (text.size() != segment.text.size())
                    break;
            }
            int leftPad = 1;
            const int free = contentWidth - used;
            if (table->columnAlignment(column) == MD::Table::AlignRight)
                leftPad += free;
            else if (table->columnAlignment(column) == MD::Table::AlignCenter)
                leftPad += free / 2;
            const int rightPad = widths[column] - leftPad - used;
            line.segments.append({QString(leftPad, QChar(' ')), MarkdownRole::Text});
            line.segments.append(clipped);
            line.segments.append({QString(std::max(0, rightPad), QChar(' ')),
                                  MarkdownRole::Text});
            line.segments.append({QStringLiteral("│"), MarkdownRole::Border});
        }
        m_renderedLines.append(line);
        if (rowIndex == 0)
            borderLine(QChar(0x255e), QChar(0x256a), QChar(0x2561), QChar(0x2550), true);
    }
    borderLine(QChar(0x2514), QChar(0x2534), QChar(0x2518), QChar(0x2500));
    m_renderedLines.append(DocumentLine{});
}

void MarkdownDocument::rebuildLinks() {
    m_links.clear();
    QSet<int> seen;
    for (int lineIndex = 0; lineIndex < m_renderedLines.size(); ++lineIndex) {
        for (const FormattedSegment &segment : m_renderedLines[lineIndex].segments) {
            if (segment.linkId < 0 || seen.contains(segment.linkId))
                continue;
            seen.insert(segment.linkId);
            m_links.append({segment.linkLabel, segment.href, lineIndex});
        }
    }
}

} // namespace qtui
