#include "MdDoc.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>
#include <algorithm>

#include <UnicodeText.h>
#include <parser.h>

namespace qtui {

namespace {
MdRole headingRole(int level) {
    switch (level) {
    case 1:
        return MdRole::Heading1;
    case 2:
        return MdRole::Heading2;
    case 3:
        return MdRole::Heading3;
    case 4:
        return MdRole::Heading4;
    case 5:
        return MdRole::Heading5;
    default:
        return MdRole::Heading6;
    }
}

QString headingPrefix(int level) {
    switch (level) {
    case 1:
        return QStringLiteral("■ ");
    case 2:
        return QStringLiteral("● ");
    case 3:
        return QStringLiteral("◆ ");
    case 4:
        return QStringLiteral("▲ ");
    case 5:
        return QStringLiteral("○ ");
    default:
        return QStringLiteral("▫ ");
    }
}
}

MdDoc::MdDoc(QObject *parent)
    : QObject(parent), m_fallbackStyle(this) {
    connect(&m_fallbackStyle, &MdDocStyle::styleChanged, this, &MdDoc::layoutDocument);
}

MdDocStyle *MdDoc::effectiveStyle() const {
    if (m_style)
        return m_style.data();
    return const_cast<MdDocStyle*>(&m_fallbackStyle);
}

MdDocStyle* MdDoc::style() const {
    return m_style.data();
}

void MdDoc::setStyle(MdDocStyle *style) {
    if (m_style == style)
        return;
    if (m_style)
        disconnect(m_style, nullptr, this, nullptr);
    m_style = style;
    if (m_style)
        connect(m_style, &MdDocStyle::styleChanged, this, &MdDoc::layoutDocument);
    emit styleChanged();
    emit renderStyleChanged();
    layoutDocument();
}

void MdDoc::setSource(const QString &source) {
    if (m_source == source)
        return;
    m_source = source;
    emit sourceChanged();
    loadSource();
}

void MdDoc::setText(const QString &text) {
    if (m_text == text)
        return;
    m_text = text;
    emit textChanged();
    parseDocument();
}

void MdDoc::setWrapWidth(int width) {
    const int clamped = std::max(1, width);
    if (m_wrapWidth == clamped)
        return;
    m_wrapWidth = clamped;
    emit wrapWidthChanged();
    layoutDocument();
}

QString MdDoc::renderStyle() const {
    const MdDocStyle *st = effectiveStyle();
    if (st->h1Format() == MdDocStyle::Heading1Format::Banner)
        return QStringLiteral("classic");
    if (st->h1Format() == MdDocStyle::Heading1Format::Plain)
        return QStringLiteral("minimal");
    return QStringLiteral("modern");
}

void MdDoc::setRenderStyle(const QString &style) {
    const QString normalized = style.trimmed().toLower();
    MdDocStyle *st = effectiveStyle();
    if (normalized == QStringLiteral("classic")) {
        st->setH1Format(MdDocStyle::Heading1Format::Banner);
        st->setHeadingFormat(MdDocStyle::HeadingFormat::Bullets);
        st->setHeadingBanners(true);
        st->setCodeBlockFormat(MdDocStyle::CodeBlockFormat::Boxed);
        st->setTableFormat(MdDocStyle::TableFormat::Boxed);
    } else if (normalized == QStringLiteral("minimal")) {
        st->setH1Format(MdDocStyle::Heading1Format::Plain);
        st->setHeadingFormat(MdDocStyle::HeadingFormat::Plain);
        st->setHeadingBanners(false);
        st->setCodeBlockFormat(MdDocStyle::CodeBlockFormat::Borderless);
        st->setTableFormat(MdDocStyle::TableFormat::Minimal);
    } else {
        st->setH1Format(MdDocStyle::Heading1Format::Badge);
        st->setHeadingFormat(MdDocStyle::HeadingFormat::Hashes);
        st->setHeadingBanners(false);
        st->setCodeBlockFormat(MdDocStyle::CodeBlockFormat::Indented);
        st->setTableFormat(MdDocStyle::TableFormat::Open);
    }
    emit renderStyleChanged();
    layoutDocument();
}

void MdDoc::setCodeBlockStyle(const QString &style) {
    const QString normalized = style.trimmed().toLower();
    if (m_codeBlockStyle == normalized)
        return;
    m_codeBlockStyle = normalized;
    emit codeBlockStyleChanged();
    layoutDocument();
}

void MdDoc::setTableStyle(const QString &style) {
    const QString normalized = style.trimmed().toLower();
    if (m_tableStyle == normalized)
        return;
    m_tableStyle = normalized;
    emit tableStyleChanged();
    layoutDocument();
}

void MdDoc::reload() {
    if (!m_source.isEmpty())
        loadSource();
    else
        parseDocument();
}

QVariantList MdDoc::tableOfContents() const {
    QVariantList result;
    result.reserve(m_toc.size());
    for (const TocItem &item : m_toc) {
        result.append(QVariantMap{{QStringLiteral("level"), item.level},
                                  {QStringLiteral("title"), item.title},
                                  {QStringLiteral("line"), item.lineIndex}});
    }
    return result;
}

QVariantList MdDoc::links() const {
    QVariantList result;
    result.reserve(m_links.size());
    for (const LinkEntry &link : m_links) {
        result.append(QVariantMap{{QStringLiteral("text"), link.text},
                                  {QStringLiteral("url"), link.url},
                                  {QStringLiteral("line"), link.lineIndex}});
    }
    return result;
}

QList<SearchMatch> MdDoc::searchMatches(const QString &query) const {
    QList<SearchMatch> matches;
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return matches;

    for (int lineIndex = 0; lineIndex < m_renderedLines.size(); ++lineIndex) {
        const DocumentLine &line = m_renderedLines[lineIndex];
        QString lineText;
        for (const FormattedSegment &segment : line.segments)
            lineText += segment.text;

        int from = 0;
        while (from < lineText.size()) {
            const int matchPos = lineText.indexOf(trimmed, from, Qt::CaseInsensitive);
            if (matchPos < 0)
                break;
            matches.append({lineIndex, matchPos, static_cast<int>(trimmed.size())});
            from = matchPos + std::max(1, static_cast<int>(trimmed.size()));
        }
    }
    return matches;
}

QVariantList MdDoc::search(const QString &query) const {
    QVariantList result;
    const auto matches = searchMatches(query);
    result.reserve(matches.size());
    for (const SearchMatch &match : matches) {
        result.append(QVariantMap{{QStringLiteral("line"), match.line},
                                  {QStringLiteral("column"), match.utf16Column},
                                  {QStringLiteral("length"), match.utf16Length}});
    }
    return result;
}

bool MdDoc::loadSource() {
    QString filePath = m_source;
    if (filePath.startsWith(QStringLiteral("qrc:")))
        filePath = filePath.mid(3);
    if (filePath.startsWith(QStringLiteral("file://")))
        filePath = QUrl(filePath).toLocalFile();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_text = QStringLiteral("# Error\n\nCould not open file: %1").arg(filePath);
        emit textChanged();
        parseDocument();
        return false;
    }

    m_text = QString::fromUtf8(file.readAll());
    emit textChanged();
    parseDocument();
    return true;
}

void MdDoc::parseDocument() {
    QTextStream stream(&m_text);
    MD::Parser parser;
    m_parsedDocument = parser.parse(stream, QStringLiteral("/"), QStringLiteral("document.md"));

    m_headingCount = 0;
    m_codeBlockCount = 0;
    m_tableCount = 0;
    m_documentTitle = QStringLiteral("Untitled Document");

    if (m_parsedDocument) {
        for (const auto &item : m_parsedDocument->items())
            collectMetadata(item);
    }

    const QStringList words = m_text.split(QRegularExpression(QStringLiteral("\\s+")),
                                           Qt::SkipEmptyParts);
    m_wordCount = words.size();

    layoutDocument();
}

void MdDoc::collectMetadata(const QSharedPointer<MD::Item> &item) {
    if (!item)
        return;

    switch (item->type()) {
    case MD::ItemType::Heading: {
        const auto heading = qSharedPointerCast<MD::Heading>(item);
        ++m_headingCount;
        if (m_headingCount == 1 && heading->text()) {
            const QString candidate = plainText(qSharedPointerCast<MD::Item>(heading->text())).trimmed();
            if (!candidate.isEmpty())
                m_documentTitle = candidate;
        }
        break;
    }
    case MD::ItemType::Code: {
        const auto code = qSharedPointerCast<MD::Code>(item);
        if (!code->isInline())
            ++m_codeBlockCount;
        break;
    }
    case MD::ItemType::Table:
        ++m_tableCount;
        break;
    case MD::ItemType::Blockquote: {
        const auto quote = qSharedPointerCast<MD::Blockquote>(item);
        for (const auto &child : quote->items())
            collectMetadata(child);
        break;
    }
    case MD::ItemType::List: {
        const auto list = qSharedPointerCast<MD::List>(item);
        for (const auto &child : list->items())
            collectMetadata(child);
        break;
    }
    case MD::ItemType::ListItem: {
        const auto listItem = qSharedPointerCast<MD::ListItem>(item);
        for (const auto &child : listItem->items())
            collectMetadata(child);
        break;
    }
    default:
        break;
    }
}

QString MdDoc::plainText(const QSharedPointer<MD::Item> &item) {
    if (!item)
        return {};

    switch (item->type()) {
    case MD::ItemType::Text:
        return qSharedPointerCast<MD::Text>(item)->text();
    case MD::ItemType::Code:
        return qSharedPointerCast<MD::Code>(item)->text();
    case MD::ItemType::Link:
    case MD::ItemType::Image: {
        const auto link = qSharedPointerCast<MD::LinkBase>(item);
        const QString nested = plainText(qSharedPointerCast<MD::Item>(link->p())).trimmed();
        if (!nested.isEmpty())
            return nested;
        if (!link->text().isEmpty())
            return link->text();
        return link->url();
    }
    case MD::ItemType::Paragraph:
    case MD::ItemType::Heading:
    case MD::ItemType::TableCell: {
        const auto block = qSharedPointerCast<MD::Block>(item);
        if (!block)
            return {};
        QString result;
        for (const auto &child : block->items())
            result += plainText(child);
        return result;
    }
    case MD::ItemType::LineBreak:
        return QStringLiteral("\n");
    default:
        return {};
    }
}

void MdDoc::layoutDocument() {
    m_renderedLines.clear();
    m_toc.clear();
    m_links.clear();
    m_nextLinkId = 1;

    if (m_parsedDocument) {
        for (const auto &item : m_parsedDocument->items())
            processItem(item);
    }
    rebuildLinks();
    emit parsedChanged();
}

void MdDoc::processItem(const QSharedPointer<MD::Item> &item, int indent,
                        int quoteDepth) {
    if (!item)
        return;

    const MdDocStyle *st = effectiveStyle();

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
            appendWrappedLine({{code->text(), MdRole::InlineCode,
                                MdRole::InlineCodeBackground, TextAttribute::Bold}},
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
        if (st->horizontalRuleFormat() == MdDocStyle::HorizontalRuleFormat::Dashed) {
            line.segments.append({st->horizontalRuleText(), MdRole::Border});
        } else {
            const int quoteWidth = std::max(1, UnicodeText::displayWidth(st->blockquotePrefix()));
            const int width = std::max(1, m_wrapWidth - indent - quoteDepth * quoteWidth);
            line.segments.append({QString(width, QChar(0x2500)), MdRole::Border});
        }
        m_renderedLines.append(line);
        m_renderedLines.append(DocumentLine{});
        break;
    }
    default: {
        const QString fallback = plainText(item).trimmed();
        if (!fallback.isEmpty())
            appendWrappedLine({{fallback, MdRole::Text}}, indent, quoteDepth);
        break;
    }
    }
}

QList<FormattedSegment> MdDoc::extractInlineSegments(
    const QSharedPointer<MD::Paragraph> &paragraph, MdRole defaultRole) {
    return extractBlockSegments(qSharedPointerCast<MD::Block>(paragraph), defaultRole);
}

QList<FormattedSegment> MdDoc::extractBlockSegments(
    const QSharedPointer<MD::Block> &block, MdRole defaultRole) {
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
            segments.append({text->text(), defaultRole, MdRole::Transparent,
                             attributes});
            break;
        }
        case MD::ItemType::Code: {
            const auto code = qSharedPointerCast<MD::Code>(item);
            segments.append({code->text(), MdRole::InlineCode,
                             MdRole::InlineCodeBackground, TextAttribute::Bold});
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
                                 ? MdRole::Success : MdRole::Link,
                             MdRole::Transparent, TextAttribute::Underline,
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

QString MdDoc::truncateToDisplayWidth(const QString &text, int width) {
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

int MdDoc::segmentsWidth(const QList<FormattedSegment> &segments) {
    int width = 0;
    for (const FormattedSegment &segment : segments)
        width += UnicodeText::displayWidth(segment.text);
    return width;
}

void MdDoc::appendWrappedLine(const QList<FormattedSegment> &segments,
                              int indent, int quoteDepth) {
    const int quoteWidth = std::max(1, UnicodeText::displayWidth(effectiveStyle()->blockquotePrefix()));
    const int maxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * quoteWidth);
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

                const QVector<Grapheme> clusters = UnicodeText::graphemes(pending);
                if (clusters.isEmpty())
                    break;
                FormattedSegment replacement = style;
                replacement.text = QStringLiteral("");
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

void MdDoc::processHeading(const QSharedPointer<MD::Heading> &heading,
                           int indent, int quoteDepth) {
    if (!heading || !heading->text())
        return;
    if (!m_renderedLines.isEmpty())
        m_renderedLines.append(DocumentLine{});

    const int level = std::clamp(heading->level(), 1, 6);
    const QString title = plainText(qSharedPointerCast<MD::Item>(heading->text())).trimmed();
    const int headingLine = m_renderedLines.size();
    m_toc.append({level, title, headingLine});

    const MdRole role = headingRole(level);
    const MdDocStyle *st = effectiveStyle();

    if (level == 1 && st->h1Format() == MdDocStyle::Heading1Format::Badge) {
        const int padding = st->h1BadgePadding();
        const QString padText = QString(padding, QLatin1Char(' '));
        QList<FormattedSegment> content = extractInlineSegments(heading->text(), MdRole::Heading1);
        QList<FormattedSegment> segments;
        if (padding > 0)
            segments.append({padText, MdRole::Heading1, MdRole::Heading1Bg, TextAttribute::Bold});
        for (FormattedSegment &seg : content) {
            seg.foreground = MdRole::Heading1;
            seg.background = MdRole::Heading1Bg;
            seg.attr = seg.attr | TextAttribute::Bold;
            segments.append(seg);
        }
        if (padding > 0)
            segments.append({padText, MdRole::Heading1, MdRole::Heading1Bg, TextAttribute::Bold});

        const int firstLine = m_renderedLines.size();
        appendWrappedLine(segments, indent, quoteDepth);
        for (int i = firstLine; i < m_renderedLines.size(); ++i) {
            m_renderedLines[i].isHeading = true;
            m_renderedLines[i].headingLevel = level;
        }
    } else {
        QString prefix;
        if (st->headingFormat() == MdDocStyle::HeadingFormat::Hashes) {
            prefix = QString(level, QLatin1Char('#')) + QStringLiteral(" ");
        } else if (st->headingFormat() == MdDocStyle::HeadingFormat::Bullets) {
            prefix = headingPrefix(level);
        }

        QList<FormattedSegment> segments;
        if (!prefix.isEmpty()) {
            segments.append({prefix, role, MdRole::Transparent, TextAttribute::Bold});
        }
        QList<FormattedSegment> content = extractInlineSegments(heading->text(), role);
        for (FormattedSegment &seg : content) {
            seg.foreground = role;
            seg.attr = seg.attr | TextAttribute::Bold;
        }
        segments.append(content);
        const int firstLine = m_renderedLines.size();
        appendWrappedLine(segments, indent, quoteDepth);
        for (int i = firstLine; i < m_renderedLines.size(); ++i) {
            m_renderedLines[i].isHeading = true;
            m_renderedLines[i].headingLevel = level;
        }

        if (st->headingBanners() && level <= 2) {
            DocumentLine banner;
            banner.indent = indent;
            banner.quoteDepth = quoteDepth;
            banner.isHeading = true;
            banner.headingLevel = level;
            const int quoteWidth = std::max(1, UnicodeText::displayWidth(st->blockquotePrefix()));
            const int available = std::max(1, m_wrapWidth - indent - quoteDepth * quoteWidth);
            const int desired = UnicodeText::displayWidth(title) + 4;
            const int length = std::min(available, std::max(level == 1 ? 20 : 15, desired));
            banner.segments.append({QString(length, level == 1 ? QChar(0x2550) : QChar(0x2500)),
                                    role});
            m_renderedLines.append(banner);
        }
    }
    m_renderedLines.append(DocumentLine{});
}

void MdDoc::processParagraph(const QSharedPointer<MD::Paragraph> &paragraph,
                             int indent, int quoteDepth,
                             const QList<FormattedSegment> &prefix) {
    if (!paragraph)
        return;
    QList<FormattedSegment> content = prefix;
    content.append(extractInlineSegments(paragraph));
    appendWrappedLine(content, indent, quoteDepth);
    m_renderedLines.append(DocumentLine{});
}

void MdDoc::processCode(const QSharedPointer<MD::Code> &code, int indent,
                        int quoteDepth) {
    if (!code)
        return;

    const MdDocStyle *st = effectiveStyle();
    const bool isBoxed = (m_codeBlockStyle == QStringLiteral("boxed"))
        || (m_codeBlockStyle == QStringLiteral("auto") && st->codeBlockFormat() == MdDocStyle::CodeBlockFormat::Boxed);

    const QString syntax = code->syntax().trimmed();
    const auto language = SyntaxHighlighter::detectLanguage(syntax);
    QStringList lines = code->text().split(QChar('\n'));
    if (!lines.isEmpty() && lines.constLast().isEmpty())
        lines.removeLast();

    const int quoteWidth = std::max(1, UnicodeText::displayWidth(st->blockquotePrefix()));

    if (!isBoxed) {
        const int codeIndent = indent + st->codeBlockIndent();
        const int contentWidth = std::max(1, m_wrapWidth - codeIndent - quoteDepth * quoteWidth);
        for (const QString &sourceLine : std::as_const(lines)) {
            DocumentLine line;
            line.indent = codeIndent;
            line.quoteDepth = quoteDepth;
            line.isCodeBlock = true;
            if (sourceLine.isEmpty()) {
                m_renderedLines.append(line);
                continue;
            }
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
            m_renderedLines.append(line);
        }
        m_renderedLines.append(DocumentLine{});
        return;
    }

    const int boxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * quoteWidth);
    if (boxWidth < 4) {
        appendWrappedLine({{code->text(), MdRole::Code,
                            MdRole::CodeBackground}}, indent, quoteDepth);
        return;
    }

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
                         MdRole::Border});
    m_renderedLines.append(top);

    const int contentWidth = boxWidth - 3;
    for (const QString &sourceLine : std::as_const(lines)) {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isCodeBlock = true;
        line.segments.append({QStringLiteral("│ "), MdRole::Border});
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
            line.segments.append({QString(contentWidth - used, QChar(' ')), MdRole::Text,
                                  MdRole::Transparent});
        line.segments.append({QStringLiteral("│"), MdRole::Border});
        m_renderedLines.append(line);
    }

    DocumentLine bottom;
    bottom.indent = indent;
    bottom.quoteDepth = quoteDepth;
    bottom.isCodeBlock = true;
    bottom.segments.append({QStringLiteral("└") + QString(boxWidth - 2, QChar(0x2500))
                                + QStringLiteral("┘"),
                            MdRole::Border});
    m_renderedLines.append(bottom);
    m_renderedLines.append(DocumentLine{});
}

void MdDoc::processBlockquote(
    const QSharedPointer<MD::Blockquote> &blockquote, int indent, int quoteDepth) {
    if (!blockquote)
        return;
    for (const auto &item : blockquote->items())
        processItem(item, indent, quoteDepth + 1);
    if (!m_renderedLines.isEmpty() && !m_renderedLines.constLast().segments.isEmpty())
        m_renderedLines.append(DocumentLine{});
}

void MdDoc::processList(const QSharedPointer<MD::List> &list, int indent,
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

void MdDoc::processListItem(const QSharedPointer<MD::ListItem> &listItem,
                            int index, bool ordered, int indent,
                            int quoteDepth) {
    if (!listItem)
        return;

    const MdDocStyle *st = effectiveStyle();
    QString marker;
    MdRole markerRole = MdRole::Link;
    if (listItem->isTaskList()) {
        marker = listItem->isChecked() ? st->taskListChecked() : st->taskListUnchecked();
        markerRole = listItem->isChecked() ? MdRole::Success : MdRole::Warning;
    } else if (ordered) {
        marker = QStringLiteral("%1. ").arg(index);
        markerRole = MdRole::Warning;
    } else {
        marker = indent == 0 ? st->listBulletLevel1()
                             : (indent == 2 ? st->listBulletLevel2() : QStringLiteral("▫ "));
    }

    bool markerUsed = false;
    const int childIndent = indent + UnicodeText::displayWidth(marker);
    for (const auto &child : listItem->items()) {
        if (!child)
            continue;
        if (!markerUsed && child->type() == MD::ItemType::Paragraph) {
            processParagraph(qSharedPointerCast<MD::Paragraph>(child), indent, quoteDepth,
                             {{marker, markerRole, MdRole::Transparent,
                               TextAttribute::Bold}});
            markerUsed = true;
        } else {
            if (!markerUsed) {
                appendWrappedLine({{marker, markerRole, MdRole::Transparent,
                                    TextAttribute::Bold}}, indent, quoteDepth);
                markerUsed = true;
            }
            processItem(child, childIndent, quoteDepth);
        }
    }
    if (!markerUsed)
        appendWrappedLine({{marker, markerRole}}, indent, quoteDepth);
}

void MdDoc::processTable(const QSharedPointer<MD::Table> &table, int indent,
                         int quoteDepth) {
    if (!table || table->rows().isEmpty() || table->columnsCount() <= 0)
        return;
    const MdDocStyle *st = effectiveStyle();
    const int columns = table->columnsCount();
    const int quoteWidth = std::max(1, UnicodeText::displayWidth(st->blockquotePrefix()));
    const int maxWidth = std::max(1, m_wrapWidth - indent - quoteDepth * quoteWidth);
    QVector<QVector<QList<FormattedSegment>>> data;
    QVector<int> widths(columns, 3);

    const bool isOpenTable = (m_tableStyle == QStringLiteral("open"))
        || (m_tableStyle == QStringLiteral("auto") && st->tableFormat() == MdDocStyle::TableFormat::Open);

    for (int rowIndex = 0; rowIndex < table->rows().size(); ++rowIndex) {
        const auto &row = table->rows()[rowIndex];
        if (!row)
            continue;
        QVector<QList<FormattedSegment>> cells(columns);
        for (int column = 0; column < std::min(columns, int(row->cells().size())); ++column) {
            if (!row->cells()[column])
                continue;
            cells[column] = extractBlockSegments(row->cells()[column],
                                                  rowIndex == 0 ? (isOpenTable ? MdRole::Text : MdRole::Warning)
                                                                : MdRole::Text);
            if (rowIndex == 0) {
                for (FormattedSegment &segment : cells[column])
                    segment.attr = segment.attr | TextAttribute::Bold;
            }
            const int segWidth = segmentsWidth(cells[column]);
            widths[column] = std::max(widths[column], segWidth + (isOpenTable ? 0 : 2));
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
                    flattened.append({QStringLiteral(" | "), MdRole::Border});
                flattened.append(row[column]);
            }
            appendWrappedLine(flattened, indent, quoteDepth);
        }
        m_renderedLines.append(DocumentLine{});
        return;
    }

    if (isOpenTable) {
        auto totalOpenWidth = [&]() {
            int total = (columns - 1) * 3;
            for (int w : std::as_const(widths))
                total += w;
            return total;
        };
        while (totalOpenWidth() > maxWidth) {
            const int widest = int(std::max_element(widths.cbegin(), widths.cend()) - widths.cbegin());
            if (widths[widest] <= 3)
                break;
            --widths[widest];
        }

        for (int rowIndex = 0; rowIndex < data.size(); ++rowIndex) {
            DocumentLine line;
            line.indent = indent;
            line.quoteDepth = quoteDepth;
            line.isTable = true;

            for (int column = 0; column < columns; ++column) {
                if (column > 0) {
                    line.segments.append({QStringLiteral(" | "), MdRole::Border});
                }
                const int colWidth = widths[column];
                QList<FormattedSegment> clipped;
                int used = 0;
                for (const FormattedSegment &segment : data[rowIndex][column]) {
                    const QString text = truncateToDisplayWidth(segment.text, colWidth - used);
                    if (text.isEmpty())
                        break;
                    FormattedSegment copy = segment;
                    copy.text = text;
                    clipped.append(copy);
                    used += UnicodeText::displayWidth(text);
                    if (text.size() != segment.text.size())
                        break;
                }
                int leftPad = 0;
                const int free = std::max(0, colWidth - used);
                if (table->columnAlignment(column) == MD::Table::AlignRight)
                    leftPad = free;
                else if (table->columnAlignment(column) == MD::Table::AlignCenter)
                    leftPad = free / 2;
                const int rightPad = free - leftPad;

                if (leftPad > 0)
                    line.segments.append({QString(leftPad, QChar(' ')), MdRole::Text});
                line.segments.append(clipped);
                if (rightPad > 0)
                    line.segments.append({QString(rightPad, QChar(' ')), MdRole::Text});
            }
            m_renderedLines.append(line);

            if (rowIndex == 0) {
                DocumentLine sepLine;
                sepLine.indent = indent;
                sepLine.quoteDepth = quoteDepth;
                sepLine.isTable = true;
                QString sepText;
                for (int column = 0; column < columns; ++column) {
                    if (column > 0)
                        sepText += QStringLiteral("+");
                    const int dashCount = widths[column] + (column == 0 || column == columns - 1 ? 1 : 2);
                    sepText += QString(dashCount, QChar('-'));
                }
                sepLine.segments.append({sepText, MdRole::Border});
                m_renderedLines.append(sepLine);
            }
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
        line.segments.append({text, MdRole::Border, MdRole::Transparent,
                              bold ? TextAttribute::Bold : TextAttribute::None});
        m_renderedLines.append(line);
    };
    borderLine(QChar(0x250c), QChar(0x252c), QChar(0x2510), QChar(0x2500));

    for (int rowIndex = 0; rowIndex < data.size(); ++rowIndex) {
        DocumentLine line;
        line.indent = indent;
        line.quoteDepth = quoteDepth;
        line.isTable = true;
        line.segments.append({QStringLiteral("│"), MdRole::Border});
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
            line.segments.append({QString(leftPad, QChar(' ')), MdRole::Text});
            line.segments.append(clipped);
            line.segments.append({QString(std::max(0, rightPad), QChar(' ')),
                                  MdRole::Text});
            line.segments.append({QStringLiteral("│"), MdRole::Border});
        }
        m_renderedLines.append(line);
        if (rowIndex == 0)
            borderLine(QChar(0x255e), QChar(0x256a), QChar(0x2561), QChar(0x2550), true);
    }
    borderLine(QChar(0x2514), QChar(0x2534), QChar(0x2518), QChar(0x2500));
    m_renderedLines.append(DocumentLine{});
}

void MdDoc::rebuildLinks() {
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
