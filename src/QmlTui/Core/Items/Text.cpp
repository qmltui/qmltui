#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QStringTokenizer>
#include <QVarLengthArray>
#include <algorithm>

namespace qmltui {

namespace {

struct MarkupState {
    TextStyle style;
    QStringView href;
};

QStringView extractTagAttribute(QStringView tagContent, QLatin1StringView attrName) {
    qsizetype pos = 0;
    while (pos < tagContent.size()) {
        const qsizetype idx = tagContent.indexOf(attrName, pos, Qt::CaseInsensitive);
        if (idx < 0) return {};
        qsizetype p = idx + attrName.size();
        while (p < tagContent.size() && tagContent[p].isSpace())
            ++p;
        if (p < tagContent.size() && tagContent[p] == u'=') {
            ++p;
            while (p < tagContent.size() && tagContent[p].isSpace())
                ++p;
            if (p < tagContent.size() && (tagContent[p] == u'\'' || tagContent[p] == u'"')) {
                const QChar quote = tagContent[p++];
                const qsizetype endQuote = tagContent.indexOf(quote, p);
                if (endQuote > p) return tagContent.sliced(p, endQuote - p);
            }
        }
        pos = idx + attrName.size();
    }
    return {};
}

qsizetype parseHtmlTag(QStringView line, qsizetype startIdx, MarkupState& current,
                       QVarLengthArray<MarkupState, 8>& styleStack) {
    if (line[startIdx] != u'<') return -1;
    const qsizetype closeIdx = line.indexOf(u'>', startIdx + 1);
    if (closeIdx < 0) return -1;

    const QStringView tagContent = line.sliced(startIdx + 1, closeIdx - startIdx - 1).trimmed();
    const bool isClose = tagContent.startsWith(u'/');
    const QStringView tagName = isClose ? tagContent.sliced(1).trimmed() : tagContent;

    const bool isFont = tagName.compare(QLatin1StringView("font"), Qt::CaseInsensitive) == 0 ||
                        tagName.startsWith(QLatin1StringView("font "), Qt::CaseInsensitive) ||
                        tagName.startsWith(QLatin1StringView("font\t"), Qt::CaseInsensitive);
    const bool isAnchor = tagName.compare(QLatin1StringView("a"), Qt::CaseInsensitive) == 0 ||
                          tagName.startsWith(QLatin1StringView("a "), Qt::CaseInsensitive) ||
                          tagName.startsWith(QLatin1StringView("a\t"), Qt::CaseInsensitive);
    const bool isBold = tagName.compare(QLatin1StringView("b"), Qt::CaseInsensitive) == 0 ||
                        tagName.compare(QLatin1StringView("strong"), Qt::CaseInsensitive) == 0;
    const bool isItalic = tagName.compare(QLatin1StringView("i"), Qt::CaseInsensitive) == 0 ||
                          tagName.compare(QLatin1StringView("em"), Qt::CaseInsensitive) == 0;
    const bool isUnderline = tagName.compare(QLatin1StringView("u"), Qt::CaseInsensitive) == 0;
    const bool isReversed =
        tagName.compare(QLatin1StringView("reversed"), Qt::CaseInsensitive) == 0;

    if (!isBold && !isItalic && !isUnderline && !isReversed && !isFont && !isAnchor) return -1;

    if (isClose) {
        if (!styleStack.isEmpty()) {
            current = styleStack.back();
            styleStack.pop_back();
        }
    } else {
        styleStack.push_back(current);
        if (isAnchor) {
            const QStringView href = extractTagAttribute(tagContent, QLatin1StringView("href"));
            if (!href.isEmpty()) current.href = href;
            current.style.setUnderline(true);
        } else if (isFont) {
            const QStringView colorStr =
                extractTagAttribute(tagContent, QLatin1StringView("color"));
            if (!colorStr.isEmpty()) {
                const Color color = Color::fromString(colorStr.toString());
                if (color.isValid()) current.style.setColor(color);
            }
        } else if (isBold) {
            current.style.setBold(true);
        } else if (isItalic) {
            current.style.setItalic(true);
        } else if (isUnderline) {
            current.style.setUnderline(true);
        } else if (isReversed) {
            current.style.setReversed(true);
        }
    }

    return closeIdx;
}

// Applies an SGR parameter list ("1;31;48;2;r;g;b") to the current TextStyle.
void applySgrParameters(QStringView paramsStr, TextStyle& currentStyle) {
    // SGR color codes name palette entries, not RGB: keep them as ANSI indices
    // so the terminal's theme applies, exactly as it would for the raw sequence.
    QVarLengthArray<int, 16> codes;
    for (QStringView part : QStringTokenizer{paramsStr, u';'}) {
        if (part.isEmpty()) {
            codes.push_back(-1);
        } else {
            bool ok = false;
            const int v = part.toInt(&ok);
            codes.push_back(ok ? v : -1);
        }
    }
    for (qsizetype pIdx = 0; pIdx < codes.size(); ++pIdx) {
        const int code = codes[pIdx];
        if (code < 0) continue;
        if (code == 0) {
            currentStyle = TextStyle{};
        } else if (code == 1) {
            currentStyle.setBold(true);
        } else if (code == 2) {
            currentStyle.setDim(true);
        } else if (code == 22) {
            currentStyle.setBold(false);
            currentStyle.setDim(false);
        } else if (code == 3) {
            currentStyle.setItalic(true);
        } else if (code == 4) {
            currentStyle.setUnderline(true);
        } else if (code == 7) {
            currentStyle.setReversed(true);
        } else if (code >= 30 && code <= 37) {
            currentStyle.setColor(Color::fromAnsiIndex(code - 30));
        } else if (code >= 40 && code <= 47) {
            currentStyle.setBackgroundColor(Color::fromAnsiIndex(code - 40));
        } else if (code >= 90 && code <= 97) {
            currentStyle.setColor(Color::fromAnsiIndex(code - 90 + 8));
        } else if (code >= 100 && code <= 107) {
            currentStyle.setBackgroundColor(Color::fromAnsiIndex(code - 100 + 8));
        } else if (code == 39) {
            currentStyle.resetColor();
        } else if (code == 49) {
            currentStyle.resetBackgroundColor();
        } else if ((code == 38 || code == 48) && pIdx + 4 < codes.size() && codes[pIdx + 1] == 2) {
            const int r = std::clamp((std::max)(0, codes[pIdx + 2]), 0, 255);
            const int g = std::clamp((std::max)(0, codes[pIdx + 3]), 0, 255);
            const int b = std::clamp((std::max)(0, codes[pIdx + 4]), 0, 255);
            const Color rgb =
                Color::fromArgb32(0xFF000000u | (static_cast<uint32_t>(r) << 16) |
                                  (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b));
            if (code == 38)
                currentStyle.setColor(rgb);
            else
                currentStyle.setBackgroundColor(rgb);
            pIdx += 4;
        }
    }
}

bool hasMarkupTrigger(QStringView line, bool parseAnsi, bool parseHtml) {
    if (!parseAnsi && !parseHtml) return false;
    for (QChar ch : line) {
        if (parseAnsi && ch == u'\033') return true;
        if (parseHtml && (ch == u'<' || ch == u'[' || ch == u'*' || ch == u'`')) return true;
    }
    return false;
}

} // namespace

Text::Text(QObject* parent) : Item(parent) {
    connect(this, &Item::widthChanged, this, [this]() {
        if (m_wrapMode != NoWrap) {
            invalidateLayout();
            updateImplicitSize();
            markDirty();
        }
    });
}

Text::~Text() = default;

void Text::setSpans(const QList<Span>& spans) {
    if (m_spans == spans) return;
    m_spans = spans;
    invalidateLayout();
    updateImplicitSize();
    emit spansChanged();
    markDirty();
}

void Text::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    invalidateLayout();
    updateImplicitSize();
    emit textChanged();
    markDirty();
}

void Text::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::resolve(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void Text::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor = Color::resolve(m_backgroundColor, 0x00000000);
    emit backgroundColorChanged();
    markDirty();
}

void Text::setBold(bool bold) {
    if (m_bold == bold) return;
    m_bold = bold;
    emit boldChanged();
    markDirty();
}

void Text::setDim(bool dim) {
    if (m_dim == dim) return;
    m_dim = dim;
    emit dimChanged();
    markDirty();
}

void Text::setItalic(bool italic) {
    if (m_italic == italic) return;
    m_italic = italic;
    emit italicChanged();
    markDirty();
}

void Text::setUnderline(bool underline) {
    if (m_underline == underline) return;
    m_underline = underline;
    emit underlineChanged();
    markDirty();
}

void Text::setReversed(bool reversed) {
    if (m_reversed == reversed) return;
    m_reversed = reversed;
    emit reversedChanged();
    markDirty();
}

void Text::setHorizontalAlignment(int alignment) {
    if (m_horizontalAlignment == alignment) return;
    m_horizontalAlignment = alignment;
    emit horizontalAlignmentChanged();
    markDirty();
}

void Text::setVerticalAlignment(int alignment) {
    if (m_verticalAlignment == alignment) return;
    m_verticalAlignment = alignment;
    emit verticalAlignmentChanged();
    markDirty();
}

void Text::setTextFormat(TextFormat format) {
    if (m_textFormat == format) return;
    m_textFormat = format;
    invalidateLayout();
    updateImplicitSize();
    emit textFormatChanged();
    markDirty();
}

void Text::setWrapMode(WrapMode mode) {
    if (m_wrapMode == mode) return;
    m_wrapMode = mode;
    invalidateLayout();
    updateImplicitSize();
    emit wrapModeChanged();
    markDirty();
}

void Text::setGradient(Gradient* gradient) {
    if (m_gradient.data() == gradient) return;
    if (m_gradient) {
        disconnect(m_gradient.data(), nullptr, this, nullptr);
    }
    m_gradient = gradient;
    if (m_gradient) {
        connect(m_gradient.data(), &Gradient::updated, this, [this]() { markDirty(); });
        connect(m_gradient.data(), &QObject::destroyed, this, [this]() {
            m_gradient = nullptr;
            emit gradientChanged();
            markDirty();
        });
    }
    emit gradientChanged();
    markDirty();
}

void Text::updateImplicitSize() {
    ensureLayout();
    int maxWidth = 0;
    for (const LineLayout& line : m_lines)
        maxWidth = (std::max)(maxWidth, line.visualWidth);
    setImplicitWidth(maxWidth);
    setImplicitHeight(static_cast<int>(m_lines.size()));
}

int Text::visualLength(const QString& line) const {
    return parseLine(line, nullptr, nullptr);
}

int Text::layoutLineRuns(QStringView line, const StyleRun* runs, qsizetype runCount,
                         qsizetype lineStartOffset, std::vector<GlyphEntry>* entries,
                         QByteArray* bytes) const {
    int visualWidth = 0;
    qsizetype runIndex = 0;
    qsizetype offset = lineStartOffset;
    GraphemeIterator iterator(line);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        while (runIndex + 1 < runCount && offset >= runs[runIndex].endOffset)
            ++runIndex;
        const StyleRun& run = runs[runIndex];
        visualWidth += appendGrapheme(cluster, run.style, run.linkIndex, entries, bytes);
        offset += cluster.utf16Length();
    }
    return visualWidth;
}

void Text::commitLine(int firstGlyph, int lineVisualWidth, int availW) const {
    const int glyphCount = static_cast<int>(m_glyphs.size()) - firstGlyph;
    if (m_wrapMode != NoWrap && availW > 0) {
        wrapGlyphs(firstGlyph, glyphCount, availW);
    } else {
        LineLayout line;
        line.firstGlyph = firstGlyph;
        line.visualWidth = lineVisualWidth;
        line.glyphCount = glyphCount;
        m_lines.push_back(line);
    }
}

int Text::parseLine(QStringView line, std::vector<GlyphEntry>* entries, QByteArray* bytes) const {
    const bool parseAnsi = m_textFormat == AutoText || m_textFormat == AnsiText;
    const bool parseHtml = m_textFormat == AutoText || m_textFormat == RichText;

    if (!hasMarkupTrigger(line, parseAnsi, parseHtml)) {
        const StyleRun plainRun{line.size(), TextStyle{}, 0};
        return layoutLineRuns(line, &plainRun, 1, 0, entries, bytes);
    }

    QString stripped;
    stripped.reserve(line.size());
    QVarLengthArray<StyleRun, 8> runs;
    MarkupState current;
    QVarLengthArray<MarkupState, 8> styleStack;

    const auto appendSlice = [&](QStringView slice, const TextStyle& style, QStringView href) {
        if (slice.isEmpty()) return;
        stripped.append(slice);
        if (!entries) return;
        const uint16_t linkIdx = registerLink(href);
        if (!runs.isEmpty() && runs.back().style == style && runs.back().linkIndex == linkIdx) {
            runs.back().endOffset = stripped.size();
        } else {
            runs.push_back(StyleRun{stripped.size(), style, linkIdx});
        }
    };

    for (qsizetype i = 0; i < line.size();) {
        // SGR ANSI escape sequence: \033[...m
        if (parseAnsi && line[i] == u'\033' && i + 1 < line.size() && line[i + 1] == u'[') {
            qsizetype j = i + 2;
            while (j < line.size() && (line[j].isDigit() || line[j] == u';'))
                ++j;
            if (j < line.size() && line[j] == u'm') {
                if (entries) applySgrParameters(line.sliced(i + 2, j - i - 2), current.style);
                i = j + 1;
                continue;
            }
        }

        // HTML-like tag
        if (parseHtml) {
            const qsizetype tagClose = parseHtmlTag(line, i, current, styleStack);
            if (tagClose != -1) {
                i = tagClose + 1;
                continue;
            }
        }

        // Markdown Link [text](url)
        if (parseHtml && line[i] == u'[') {
            const qsizetype closeBracket = line.indexOf(u']', i);
            if (closeBracket != -1 && closeBracket + 1 < line.size() &&
                line[closeBracket + 1] == u'(') {
                const qsizetype closeParen = line.indexOf(u')', closeBracket + 2);
                if (closeParen != -1) {
                    const QStringView linkText = line.sliced(i + 1, closeBracket - i - 1);
                    const QStringView linkUrl =
                        line.sliced(closeBracket + 2, closeParen - closeBracket - 2);
                    TextStyle linkStyle = current.style;
                    linkStyle.setUnderline(true);
                    appendSlice(linkText, linkStyle, linkUrl);
                    i = closeParen + 1;
                    continue;
                }
            }
        }

        // Markdown **bold**
        if (parseHtml && i + 1 < line.size() && line[i] == u'*' && line[i + 1] == u'*') {
            if (current.style.hasBold() && current.style.bold())
                current.style.resetBold();
            else
                current.style.setBold(true);
            i += 2;
            continue;
        }

        // Markdown `code`
        if (parseHtml && line[i] == u'`') {
            if (current.style.hasBold() && current.style.bold())
                current.style.resetBold();
            else
                current.style.setBold(true);
            i += 1;
            continue;
        }

        qsizetype nextMarkup = i + 1;
        while (nextMarkup < line.size()) {
            const QChar ch = line[nextMarkup];
            if (parseAnsi && ch == u'\033') break;
            if (parseHtml && (ch == u'<' || ch == u'[' || ch == u'*' || ch == u'`')) break;
            ++nextMarkup;
        }
        appendSlice(line.sliced(i, nextMarkup - i), current.style, current.href);
        i = nextMarkup;
    }

    if (runs.isEmpty()) {
        const StyleRun emptyRun{stripped.size(), TextStyle{}, 0};
        return layoutLineRuns(stripped, &emptyRun, 1, 0, entries, bytes);
    }
    return layoutLineRuns(stripped, runs.constData(), runs.size(), 0, entries, bytes);
}

TextAttribute Text::baseAttributes() const {
    TextAttribute attributes = TextAttribute::None;
    if (m_dim) attributes = attributes | TextAttribute::Dim;
    if (m_bold) attributes = attributes | TextAttribute::Bold;
    if (m_italic) attributes = attributes | TextAttribute::Italic;
    if (m_underline) attributes = attributes | TextAttribute::Underline;
    if (m_reversed) attributes = attributes | TextAttribute::Reversed;
    return attributes;
}

int Text::appendGrapheme(const GraphemeRef& cluster, const TextStyle& style, uint16_t linkIndex,
                         std::vector<GlyphEntry>* entries, QByteArray* bytes) const {
    const int advance = (std::max)(0, cluster.width);
    int cellWidth = advance;
    if (advance == 0) {
        GraphemeIterator sanitized(cluster.utf16, /*sanitizeControls=*/true);
        GraphemeRef part;
        cellWidth = 0;
        while (sanitized.next(part))
            cellWidth += part.width;
    }

    if (entries) {
        GlyphEntry entry;
        entry.style = style;
        entry.advance = static_cast<uint8_t>((std::min)(advance, 255));
        entry.cellWidth = static_cast<uint8_t>((std::min)(cellWidth, 255));
        entry.linkIndex = linkIndex;
        entry.glyphOffset = static_cast<uint32_t>(bytes->size());
        char encoded[64];
        const int length = UnicodeText::encodeUtf8(cluster.utf16, encoded, sizeof encoded,
                                                   /*sanitizeControls=*/true);
        if (length >= 0) {
            bytes->append(encoded, length);
            entry.glyphLength = static_cast<uint32_t>(length);
        } else {
            const QByteArray fallback = UnicodeText::toUtf8(cluster.utf16, true);
            bytes->append(fallback);
            entry.glyphLength = static_cast<uint32_t>(fallback.size());
        }
        entries->push_back(entry);
    }

    return advance;
}

bool Text::isSpaceGlyph(const GlyphEntry& g) const {
    if (g.glyphLength == 1 && g.glyphOffset < static_cast<uint32_t>(m_glyphBytes.size())) {
        const char ch = m_glyphBytes.at(static_cast<int>(g.glyphOffset));
        return ch == ' ' || ch == '\t';
    }
    return false;
}

uint16_t Text::registerLink(QStringView href) const {
    if (href.isEmpty()) return 0;
    for (size_t idx = 0; idx < m_links.size(); ++idx) {
        if (m_links[idx] == href) return static_cast<uint16_t>(idx + 1);
    }
    m_links.push_back(href.toString());
    return static_cast<uint16_t>(m_links.size());
}

int Text::contentStartY(int availH, int lineCount) const {
    int y = topPadding();
    if (m_verticalAlignment & Qt::AlignVCenter) {
        y += (availH - lineCount) / 2;
    } else if (m_verticalAlignment & Qt::AlignBottom) {
        y += (availH - lineCount);
    }
    return y;
}

int Text::lineStartX(int leftLimit, int availW, int visLen) const {
    int x = leftLimit;
    if (m_horizontalAlignment & Qt::AlignRight) {
        x += (availW - visLen);
    } else if (m_horizontalAlignment & Qt::AlignHCenter) {
        x += (availW - visLen) / 2;
    }
    return x;
}

void Text::wrapGlyphs(int startGlyph, int glyphCount, int availW) const {
    if (availW <= 0 || glyphCount <= 0) {
        LineLayout line;
        line.firstGlyph = startGlyph;
        line.glyphCount = glyphCount;
        line.visualWidth = 0;
        for (int i = startGlyph; i < startGlyph + glyphCount; ++i)
            line.visualWidth += m_glyphs[i].advance;
        m_lines.push_back(line);
        return;
    }

    const int endGlyph = startGlyph + glyphCount;
    int currentLineFirst = -1;
    int currentLineCount = 0;
    int currentLineWidth = 0;

    auto flushLine = [this, &currentLineFirst, &currentLineCount, &currentLineWidth]() {
        if (currentLineCount > 0) {
            LineLayout line;
            line.firstGlyph = currentLineFirst;
            line.glyphCount = currentLineCount;
            line.visualWidth = currentLineWidth;
            m_lines.push_back(line);
            currentLineFirst = -1;
            currentLineCount = 0;
            currentLineWidth = 0;
        }
    };

    if (m_wrapMode == WrapAnywhere) {
        for (int i = startGlyph; i < endGlyph; ++i) {
            const int adv = m_glyphs[i].advance;
            if (currentLineWidth + adv > availW && currentLineCount > 0) {
                flushLine();
            }
            if (currentLineCount == 0) {
                currentLineFirst = i;
            }
            currentLineCount++;
            currentLineWidth += adv;
        }
        flushLine();
        return;
    }

    auto appendWord = [&](int wordStart, int wordCount, int wordWidth) {
        if (wordWidth <= availW) {
            currentLineFirst = wordStart;
            currentLineCount = wordCount;
            currentLineWidth = wordWidth;
            return;
        }
        int wIdx = wordStart;
        while (wIdx < wordStart + wordCount) {
            int chunkStart = wIdx;
            int chunkWidth = 0;
            int chunkCount = 0;
            while (wIdx < wordStart + wordCount && chunkWidth + m_glyphs[wIdx].advance <= availW) {
                chunkWidth += m_glyphs[wIdx].advance;
                chunkCount++;
                wIdx++;
            }
            if (chunkCount == 0 && wIdx < wordStart + wordCount) {
                chunkWidth += m_glyphs[wIdx].advance;
                chunkCount++;
                wIdx++;
            }
            if (wIdx < wordStart + wordCount) {
                LineLayout line;
                line.firstGlyph = chunkStart;
                line.glyphCount = chunkCount;
                line.visualWidth = chunkWidth;
                m_lines.push_back(line);
            } else {
                currentLineFirst = chunkStart;
                currentLineCount = chunkCount;
                currentLineWidth = chunkWidth;
            }
        }
    };

    // WordWrap or Wrap:
    int i = startGlyph;
    while (i < endGlyph) {
        int spaceStart = i;
        int spaceWidth = 0;
        while (i < endGlyph && isSpaceGlyph(m_glyphs[i])) {
            spaceWidth += m_glyphs[i].advance;
            ++i;
        }
        int spaceCount = i - spaceStart;

        if (i >= endGlyph) {
            if (currentLineCount == 0) {
                LineLayout line;
                line.firstGlyph = spaceStart;
                line.glyphCount = spaceCount;
                line.visualWidth = spaceWidth;
                m_lines.push_back(line);
            }
            break;
        }

        int wordStart = i;
        int wordWidth = 0;
        while (i < endGlyph && !isSpaceGlyph(m_glyphs[i])) {
            wordWidth += m_glyphs[i].advance;
            ++i;
        }
        int wordCount = i - wordStart;

        if (currentLineCount == 0) {
            appendWord(wordStart, wordCount, wordWidth);
        } else if (currentLineWidth + spaceWidth + wordWidth <= availW) {
            currentLineCount += (spaceCount + wordCount);
            currentLineWidth += (spaceWidth + wordWidth);
        } else {
            flushLine();
            appendWord(wordStart, wordCount, wordWidth);
        }
    }
    flushLine();
}

void Text::layoutSpans() const {
    // Segment the whole logical line so a style boundary never breaks a grapheme.
    const int availW = width() - leftPadding() - rightPadding();
    if (m_spans.size() == 1) {
        const QStringView view(m_spans[0].text());
        const StyleRun singleRun{view.size(), m_spans[0].style(), 0};
        qsizetype lineStart = 0;
        do {
            qsizetype lineEnd = view.indexOf(u'\n', lineStart);
            if (lineEnd < 0) lineEnd = view.size();
            const int firstGlyph = static_cast<int>(m_glyphs.size());
            const int lineVisualWidth =
                layoutLineRuns(view.sliced(lineStart, lineEnd - lineStart), &singleRun, 1,
                               lineStart, &m_glyphs, &m_glyphBytes);
            commitLine(firstGlyph, lineVisualWidth, availW);
            lineStart = lineEnd + 1;
        } while (lineStart <= view.size());
        return;
    }

    QString content;
    QVarLengthArray<StyleRun, 8> runs;
    runs.reserve(m_spans.size());
    for (const Span& span : m_spans) {
        content += span.text();
        runs.push_back(StyleRun{content.size(), span.style(), 0});
    }
    const QStringView view(content);
    qsizetype lineStart = 0;
    do {
        qsizetype lineEnd = view.indexOf(u'\n', lineStart);
        if (lineEnd < 0) lineEnd = view.size();
        const int firstGlyph = static_cast<int>(m_glyphs.size());
        const int lineVisualWidth =
            layoutLineRuns(view.sliced(lineStart, lineEnd - lineStart), runs.constData(),
                           runs.size(), lineStart, &m_glyphs, &m_glyphBytes);
        commitLine(firstGlyph, lineVisualWidth, availW);
        lineStart = lineEnd + 1;
    } while (lineStart <= view.size());
}

void Text::ensureLayout() const {
    if (!m_layoutDirty) return;
    m_glyphs.clear();
    m_lines.clear();
    m_glyphBytes.clear();
    m_links.clear();

    if (!m_spans.isEmpty()) {
        layoutSpans();
        m_layoutDirty = false;
        return;
    }

    const int availW = width() - leftPadding() - rightPadding();
    m_lines.reserve(static_cast<size_t>(m_text.count(u'\n') + 1));
    for (QStringView line : QStringTokenizer{m_text, u'\n'}) {
        const int startGlyph = static_cast<int>(m_glyphs.size());
        const int lineVisualWidth = parseLine(line, &m_glyphs, &m_glyphBytes);
        commitLine(startGlyph, lineVisualWidth, availW);
    }
    m_layoutDirty = false;
}

void Text::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible) return;

    ensureLayout();

    const int lp = leftPadding();
    const int rp = rightPadding();
    const int tp = topPadding();
    const int bp = bottomPadding();

    const int availW = width() - lp - rp;
    const int availH = height() - tp - bp;
    const int lineCount = static_cast<int>(m_lines.size());

    int currentY = absY + contentStartY(availH, lineCount);

    const bool useGradient = (m_gradient && !m_gradient->stopsList().isEmpty());
    const int rightLimit = absX + width() - rp;
    const int leftLimit = absX + lp;
    const char* glyphBytes = m_glyphBytes.constData();
    const TextAttribute baseAttr = baseAttributes();
    const TextAttribute disabledAttr =
        !isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None;
    const bool fillLineSpan = m_reversed || ((m_resolvedBackgroundColor >> 24) != 0);

    for (int lineIdx = 0; lineIdx < lineCount && lineIdx < availH; ++lineIdx) {
        const LineLayout& line = m_lines[lineIdx];
        if ((line.glyphCount > 0 || fillLineSpan) && currentY >= absY + tp &&
            currentY < absY + height() - bp && currentY >= 0 && currentY < buffer.height()) {
            if (fillLineSpan && clipRect.y <= currentY && currentY < clipRect.y + clipRect.height) {
                const TextAttribute fillAttr = baseAttr | disabledAttr;
                const int fillStart = (std::max)(leftLimit, clipRect.x);
                const int fillEnd = (std::min)(rightLimit, clipRect.x + clipRect.width);
                for (int fillX = fillStart; fillX < fillEnd; ++fillX) {
                    buffer.setGrapheme(fillX, currentY, QByteArrayView(" ", 1), 1, m_resolvedColor,
                                       m_resolvedBackgroundColor, fillAttr);
                }
            }
            const int visLen = line.visualWidth;
            int cx = lineStartX(leftLimit, availW, visLen);
            int charIndexInLine = 0;
            const GlyphEntry* entry = m_glyphs.data() + line.firstGlyph;
            const GlyphEntry* end = entry + line.glyphCount;
            for (; entry != end; ++entry) {
                uint32_t charFgColor = entry->style.resolveForeground(m_resolvedColor);
                if (useGradient) {
                    const qreal t =
                        (visLen > 1) ? static_cast<qreal>(charIndexInLine) / (visLen - 1) : 0.0;
                    charFgColor = m_gradient->colorAt(t);
                }

                if (cx >= leftLimit && cx + entry->advance <= rightLimit &&
                    clipRect.contains(cx, currentY) &&
                    clipRect.contains(cx + (std::max)(1, int(entry->cellWidth)) - 1, currentY)) {
                    const uint8_t linkId =
                        (entry->linkIndex > 0 && entry->linkIndex <= m_links.size())
                            ? buffer.registerLink(m_links[entry->linkIndex - 1])
                            : 0;
                    buffer.setActiveLinkId(linkId);
                    buffer.setGrapheme(
                        cx, currentY,
                        QByteArrayView(glyphBytes + entry->glyphOffset, entry->glyphLength),
                        entry->cellWidth, charFgColor,
                        entry->style.resolveBackground(m_resolvedBackgroundColor),
                        entry->style.resolveAttributes(baseAttr) | disabledAttr);
                }
                cx += entry->advance;
                charIndexInLine += entry->advance;
            }
            buffer.setActiveLinkId(0);
        }
        currentY++;
    }
}

void Text::mousePressEvent(MouseEvent* event) {
    if (m_links.empty()) {
        Item::mousePressEvent(event);
        return;
    }

    ensureLayout();

    const int lp = leftPadding();
    const int rp = rightPadding();
    const int tp = topPadding();
    const int bp = bottomPadding();

    const int availW = width() - lp - rp;
    const int availH = height() - tp - bp;
    const int lineCount = static_cast<int>(m_lines.size());

    const int startY = contentStartY(availH, lineCount);
    const int clickX = event->x();
    const int clickY = event->y();
    const int lineIdx = (clickY - startY);

    if (lineIdx >= 0 && lineIdx < lineCount) {
        const LineLayout& line = m_lines[lineIdx];
        if (line.glyphCount > 0) {
            int curX = lineStartX(lp, availW, line.visualWidth);
            const GlyphEntry* entry = m_glyphs.data() + line.firstGlyph;
            const GlyphEntry* end = entry + line.glyphCount;
            for (; entry != end; ++entry) {
                if (clickX >= curX && clickX < curX + entry->advance) {
                    if (entry->linkIndex > 0 && entry->linkIndex <= m_links.size()) {
                        const QString href = m_links[entry->linkIndex - 1];
                        emit linkActivated(href);
                        event->setAccepted(true);
                        return;
                    }
                    break;
                }
                curX += entry->advance;
            }
        }
    }

    Item::mousePressEvent(event);
}

} // namespace qmltui
