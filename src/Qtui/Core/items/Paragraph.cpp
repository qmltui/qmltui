#include <Qtui/Core/items/Paragraph.h>
#include <Qtui/Core/primitives/ColorMapper.h>
#include <Qtui/Core/engine/MouseEvent.h>
#include <algorithm>
#include <QStack>
#include <QRegularExpression>
#include <Qtui/Core/primitives/UnicodeText.h>

namespace qtui {

Paragraph::Paragraph(QObject *parent) : Item(parent) {
    setImplicitWidth(30);
    setImplicitHeight(5);
}

void Paragraph::setText(const QString &text) {
    if (m_text == text) return;
    m_text = text;
    invalidateTextCache();
    emit textChanged();
    markDirty();
}

void Paragraph::setTextFormat(TextFormat format) {
    if (m_textFormat == format) return;
    m_textFormat = format;
    invalidateTextCache();
    emit textFormatChanged();
    markDirty();
}

void Paragraph::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    invalidateTextCache();
    emit colorChanged();
    markDirty();
}

void Paragraph::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor = ColorMapper::fromString(m_backgroundColor, 0x00000000);
    invalidateTextCache();
    emit backgroundColorChanged();
    markDirty();
}

void Paragraph::setAlignment(const QString &align) {
    if (m_alignment == align) return;
    m_alignment = align;
    emit alignmentChanged();
    markDirty();
}

void Paragraph::setWrapMode(bool wrap) {
    if (m_wrapMode == wrap) return;
    m_wrapMode = wrap;
    invalidateTextCache();
    emit wrapModeChanged();
    markDirty();
}

void Paragraph::setScrollY(int scroll) {
    int clamped = std::max(0, scroll);
    if (m_scrollY == clamped) return;
    m_scrollY = clamped;
    emit scrollYChanged();
    markDirty();
}


FormattedLine Paragraph::parseRichLine(const QString &line) const {
    FormattedLine result;
    if (line.isEmpty()) return result;

    bool isRich = (m_textFormat == RichText) || (m_textFormat == AutoText && (line.contains('<') || line.contains('[') || line.contains("**") || line.contains('`')));
    if (!isRich) {
        for (const Grapheme &cluster : UnicodeText::graphemes(line))
            result.append(FormattedChar{QString::fromUtf8(cluster.utf8), cluster.width,
                                        m_resolvedColor, m_resolvedBackgroundColor,
                                        TextAttribute::None, ""});
        return result;
    }

    QStack<uint32_t> fgStack;
    QStack<uint32_t> bgStack;
    fgStack.push(m_resolvedColor);
    bgStack.push(m_resolvedBackgroundColor);

    QString activeHref;
    bool bold = false;
    bool code = false;
    bool italic = false;
    bool underline = false;

    int i = 0;
    int len = line.length();

    while (i < len) {
        // Parse HTML Tags: <a href="..."> </a> <font color="..."> <b> <i> <u>
        if (line[i] == '<') {
            int closeIdx = line.indexOf('>', i);
            if (closeIdx != -1) {
                QString tag = line.mid(i + 1, closeIdx - i - 1).trimmed();
                QString lowerTag = tag.toLower();

                if (lowerTag.startsWith("a ") || lowerTag == "a") {
                    if (lowerTag.contains("href=")) {
                        int startQuote = tag.indexOf("href=") + 5;
                        if (startQuote < tag.length() && (tag[startQuote] == '"' || tag[startQuote] == '\'')) startQuote++;
                        int endQuote = tag.indexOf(QRegularExpression("[\"'>\\s]"), startQuote);
                        if (endQuote == -1) endQuote = tag.length();
                        activeHref = tag.mid(startQuote, endQuote - startQuote);
                        fgStack.push(fgStack.top()); // Links keep the configured text color.
                    }
                } else if (lowerTag == "/a") {
                    activeHref.clear();
                    if (fgStack.size() > 1) fgStack.pop();
                } else if (lowerTag == "b") bold = true;
                else if (lowerTag == "/b") bold = false;
                else if (lowerTag == "i") italic = true;
                else if (lowerTag == "/i") italic = false;
                else if (lowerTag == "u") underline = true;
                else if (lowerTag == "/u") underline = false;
                else if (lowerTag.startsWith("font")) {
                    if (lowerTag.contains("color=")) {
                        int startQuote = tag.indexOf("color=") + 6;
                        if (startQuote < tag.length() && (tag[startQuote] == '"' || tag[startQuote] == '\'')) startQuote++;
                        int endQuote = tag.indexOf(QRegularExpression("[\"'>\\s]"), startQuote);
                        if (endQuote == -1) endQuote = tag.length();
                        QString colName = tag.mid(startQuote, endQuote - startQuote);
                        fgStack.push(ColorMapper::fromString(colName, fgStack.top()));
                    }
                } else if (lowerTag == "/font") {
                    if (fgStack.size() > 1) fgStack.pop();
                }

                i = closeIdx + 1;
                continue;
            }
        }

        // Parse Markdown Links [text](url)
        if (line[i] == '[') {
            int closeBracket = line.indexOf(']', i);
            if (closeBracket != -1 && closeBracket + 1 < len && line[closeBracket + 1] == '(') {
                int closeParen = line.indexOf(')', closeBracket + 2);
                if (closeParen != -1) {
                    QString linkText = line.mid(i + 1, closeBracket - i - 1);
                    QString linkUrl = line.mid(closeBracket + 2, closeParen - closeBracket - 2);

                    for (const Grapheme &cluster : UnicodeText::graphemes(linkText))
                        result.append(FormattedChar{QString::fromUtf8(cluster.utf8), cluster.width,
                                                    fgStack.top(), m_resolvedBackgroundColor,
                                                    TextAttribute::Underline, linkUrl});
                    i = closeParen + 1;
                    continue;
                }
            }
        }

        // Parse Markdown **bold**
        if (i + 1 < len && line[i] == '*' && line[i + 1] == '*') {
            bold = !bold;
            i += 2;
            continue;
        }

        // Parse Markdown `code`
        if (line[i] == '`') {
            code = !code;
            i += 1;
            continue;
        }

        TextAttribute attr = TextAttribute::None;
        if (bold || code) attr = attr | TextAttribute::Bold;
        if (italic || underline || !activeHref.isEmpty()) attr = attr | TextAttribute::Underline;

        const int next = UnicodeText::nextBoundary(line, i);
        const QString cluster = line.mid(i, next - i);
        result.append(FormattedChar{cluster, UnicodeText::displayWidth(cluster),
                                    fgStack.top(), bgStack.top(), attr, activeHref});
        i = next;
    }

    return result;
}

QList<FormattedLine> Paragraph::wordWrapFormatted(const FormattedLine &rawLine, int maxW) const {
    if (maxW <= 0 || rawLine.isEmpty()) return {};

    auto lineWidth = [](const FormattedLine &line) {
        int width = 0;
        for (const FormattedChar &character : line) width += character.width;
        return width;
    };

    QList<FormattedLine> result;
    if (!m_wrapMode || lineWidth(rawLine) <= maxW) {
        result.append(rawLine);
        return result;
    }

    FormattedLine currentLine;
    int i = 0;

    while (i < rawLine.size()) {
        int wordStart = i;
        while (i < rawLine.size() && rawLine[i].text != " ") i++;

        FormattedLine word;
        for (int k = wordStart; k < i; ++k) word.append(rawLine[k]);

        if (currentLine.isEmpty()) {
            currentLine = word;
        } else if (lineWidth(currentLine) + 1 + lineWidth(word) <= maxW) {
            currentLine.append(FormattedChar{QStringLiteral(" "), 1, m_resolvedColor,
                                              m_resolvedBackgroundColor, TextAttribute::None, ""});
            currentLine.append(word);
        } else {
            result.append(currentLine);
            currentLine = word;
        }

        if (i < rawLine.size() && rawLine[i].text == " ") {
            i++;
        }
    }

    if (!currentLine.isEmpty()) {
        result.append(currentLine);
    }

    return result;
}

QList<FormattedLine> Paragraph::getRenderedLines(int availW) const {
    if (m_cachedWidth == availW)
        return m_cachedLines;
    QStringList rawParagraphs = m_text.split('\n');
    QList<FormattedLine> wrappedLines;

    for (const QString &rawP : rawParagraphs) {
        FormattedLine parsedLine = parseRichLine(rawP);
        QList<FormattedLine> lines = wordWrapFormatted(parsedLine, availW);
        wrappedLines.append(lines);
    }
    m_cachedWidth = availW;
    m_cachedLines = wrappedLines;
    return m_cachedLines;
}

void Paragraph::invalidateTextCache() {
    m_cachedWidth = -1;
    m_cachedLines.clear();
}

void Paragraph::paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const {
    Q_UNUSED(clipRect);
    if (!m_visible || m_text.isEmpty()) return;

    const int availW = width();
    const int availH = height();
    const int startX = absX;
    const int startY = absY;

    if (availW <= 0 || availH <= 0) return;

    QList<FormattedLine> wrappedLines = getRenderedLines(availW);

    for (int idx = 0; idx < availH && (m_scrollY + idx) < wrappedLines.size(); ++idx) {
        const FormattedLine &line = wrappedLines[m_scrollY + idx];
        int lineY = startY + idx;
        int lineLen = 0;
        for (const FormattedChar &character : line)
            lineLen += character.width;
        lineLen = std::min(availW, lineLen);

        int curX = startX;
        if (m_alignment == "right") {
            curX = startX + (availW - lineLen);
        } else if (m_alignment == "center") {
            curX = startX + ((availW - lineLen) / 2);
        }

        int usedWidth = 0;
        for (const FormattedChar &fc : line) {
            if (usedWidth + fc.width > lineLen) break;
            buffer.writeText(curX + usedWidth, lineY, fc.text, fc.fg, fc.bg, fc.attr
                | (!isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None));
            usedWidth += fc.width;
        }
    }
}

void Paragraph::mousePressEvent(MouseEvent *event) {
    int availW = width();
    if (availW <= 0) return;

    int clickX = event->x();
    int clickY = event->y() + m_scrollY;

    QList<FormattedLine> lines = getRenderedLines(availW);
    if (clickY >= 0 && clickY < lines.size()) {
        const FormattedLine &line = lines[clickY];
        int startX = 0;
        if (m_alignment == "right") {
            int lineWidth = 0;
            for (const FormattedChar &character : line) lineWidth += character.width;
            startX = availW - lineWidth;
        } else if (m_alignment == "center") {
            int lineWidth = 0;
            for (const FormattedChar &character : line) lineWidth += character.width;
            startX = (availW - lineWidth) / 2;
        }

        const int column = clickX - startX;
        int offset = 0;
        for (const FormattedChar &character : line) {
            if (column >= offset && column < offset + character.width &&
                !character.href.isEmpty()) {
                emit linkActivated(character.href);
                event->setAccepted(true);
                break;
            }
            offset += character.width;
        }
    }
}

} // namespace qtui
