#include <Qamui/Core/items/Text.h>
#include <Qamui/Core/items/Gradient.h>
#include <Qamui/Core/primitives/UnicodeText.h>
#include <Qamui/Core/primitives/ColorMapper.h>
#include <QRegularExpression>
#include <QStringList>
#include <algorithm>
#include <vector>

namespace qamui {

struct TextStyle {
    uint32_t fgColor = 0xFFFFFFFF;
    uint32_t bgColor = 0x00000000;
    TextAttribute attributes = TextAttribute::None;
};

static int parseHtmlTag(const QString& line, int startIdx, TextStyle& currentStyle, std::vector<TextStyle>& styleStack) {
    if (line[startIdx] != '<') return -1;
    int closeIdx = line.indexOf('>', startIdx + 1);
    if (closeIdx == -1) return -1;

    QString tagContent = line.mid(startIdx + 1, closeIdx - startIdx - 1).trimmed();
    QString tagLower = tagContent.toLower();

    bool isClose = tagLower.startsWith('/');
    QString tagName = isClose ? tagLower.mid(1).trimmed() : tagLower;

    // Check if it's a font tag (might have attributes)
    bool isFont = false;
    if (tagName.startsWith("font")) {
        if (tagName == "font" || tagName.startsWith("font ")) {
            isFont = true;
        }
    }

    bool isValidTag = (tagName == "b" || tagName == "strong" || tagName == "i" || tagName == "em" ||
                       tagName == "u" || tagName == "reversed" || isFont);

    if (!isValidTag) return -1;

    if (isClose) {
        if (!styleStack.empty()) {
            currentStyle = styleStack.back();
            styleStack.pop_back();
        }
    } else {
        styleStack.push_back(currentStyle);
        if (isFont) {
            static const QRegularExpression regex("color\\s*=\\s*['\"]([^'\"]+)['\"]", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = regex.match(tagContent);
            if (match.hasMatch()) {
                QString colorStr = match.captured(1);
                currentStyle.fgColor = ColorMapper::fromString(colorStr, currentStyle.fgColor);
            }
        } else if (tagName == "b" || tagName == "strong") {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Bold;
        } else if (tagName == "i" || tagName == "em") {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Italic;
        } else if (tagName == "u") {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Underline;
        } else if (tagName == "reversed") {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Reversed;
        }
    }

    return closeIdx;
}

// Applies an SGR parameter list ("1;31;48;2;r;g;b") to the current style.
static void applySgrParameters(const QString& paramsStr, TextStyle& currentStyle,
                               const TextStyle& baseStyle) {
    // SGR color codes name palette entries, not RGB: keep them as ANSI indices
    // so the terminal's theme applies, exactly as it would for the raw sequence.
    const auto ansiIndex = [](int index) { return Color::fromAnsiIndex(index).argb32(); };
    const QStringList params = paramsStr.split(';');
    for (int pIdx = 0; pIdx < params.size(); ++pIdx) {
        if (params[pIdx].isEmpty()) continue;
        const int code = params[pIdx].toInt();
        if (code == 0) {
            currentStyle = baseStyle;
        } else if (code == 1) {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Bold;
        } else if (code == 2) {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Dim;
        } else if (code == 22) {
            currentStyle.attributes = static_cast<TextAttribute>(static_cast<uint8_t>(currentStyle.attributes)
                & ~static_cast<uint8_t>(TextAttribute::Bold | TextAttribute::Dim));
        } else if (code == 3) {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Italic;
        } else if (code == 4) {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Underline;
        } else if (code == 7) {
            currentStyle.attributes = currentStyle.attributes | TextAttribute::Reversed;
        } else if (code >= 30 && code <= 37) {
            currentStyle.fgColor = ansiIndex(code - 30);
        } else if (code >= 40 && code <= 47) {
            currentStyle.bgColor = ansiIndex(code - 40);
        } else if (code >= 90 && code <= 97) {
            currentStyle.fgColor = ansiIndex(code - 90 + 8);
        } else if (code >= 100 && code <= 107) {
            currentStyle.bgColor = ansiIndex(code - 100 + 8);
        } else if (code == 39) {
            currentStyle.fgColor = baseStyle.fgColor;
        } else if (code == 49) {
            currentStyle.bgColor = baseStyle.bgColor;
        } else if (code == 38 && pIdx + 4 < params.size() && params[pIdx + 1].toInt() == 2) {
            const int r = std::clamp(params[pIdx + 2].toInt(), 0, 255);
            const int g = std::clamp(params[pIdx + 3].toInt(), 0, 255);
            const int b = std::clamp(params[pIdx + 4].toInt(), 0, 255);
            currentStyle.fgColor = 0xFF000000u | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            pIdx += 4;
        } else if (code == 48 && pIdx + 4 < params.size() && params[pIdx + 1].toInt() == 2) {
            const int r = std::clamp(params[pIdx + 2].toInt(), 0, 255);
            const int g = std::clamp(params[pIdx + 3].toInt(), 0, 255);
            const int b = std::clamp(params[pIdx + 4].toInt(), 0, 255);
            currentStyle.bgColor = 0xFF000000u | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            pIdx += 4;
        }
    }
}

Text::Text(QObject *parent) : Item(parent) {
}

Text::~Text() = default;

void Text::setSpans(const QList<Span> &spans) {
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
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    invalidateLayout();   // the base colour is the SGR reset target
    emit colorChanged();
    markDirty();
}

void Text::setBold(bool bold) {
    if (m_bold == bold) return;
    m_bold = bold;
    invalidateLayout();
    emit boldChanged();
    markDirty();
}

void Text::setDim(bool dim) {
    if (m_dim == dim) return;
    m_dim = dim;
    invalidateLayout();
    emit dimChanged();
    markDirty();
}

void Text::setItalic(bool italic) {
    if (m_italic == italic) return;
    m_italic = italic;
    invalidateLayout();
    emit italicChanged();
    markDirty();
}

void Text::setUnderline(bool underline) {
    if (m_underline == underline) return;
    m_underline = underline;
    invalidateLayout();
    emit underlineChanged();
    markDirty();
}

void Text::setReversed(bool reversed) {
    if (m_reversed == reversed) return;
    m_reversed = reversed;
    invalidateLayout();
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
    for (const LineLayout &line : m_lines)
        maxWidth = (std::max)(maxWidth, line.visualWidth);
    setImplicitWidth(maxWidth);
    setImplicitHeight(static_cast<int>(m_lines.size()));
}

int Text::visualLength(const QString &line) const {
    return parseLine(line, nullptr, nullptr);
}

int Text::parseLine(const QString &line, std::vector<GlyphEntry> *entries,
                    QByteArray *bytes) const {
    TextStyle baseStyle;
    baseStyle.fgColor = m_resolvedColor;
    baseStyle.bgColor = 0x00000000;
    baseStyle.attributes = baseAttributes();

    const bool parseAnsi = m_textFormat == AutoText || m_textFormat == AnsiText;
    const bool parseHtml = m_textFormat == AutoText || m_textFormat == RichText;

    TextStyle currentStyle = baseStyle;
    std::vector<TextStyle> styleStack;
    int visualWidth = 0;
    const QStringView view(line);

    for (int i = 0; i < line.size(); ) {
        // SGR ANSI escape sequence: \033[...m
        if (parseAnsi && line[i] == '\033' && i + 1 < line.size() && line[i + 1] == '[') {
            int j = i + 2;
            while (j < line.size() && (line[j].isDigit() || line[j] == ';'))
                j++;
            if (j < line.size() && line[j] == 'm') {
                if (entries)
                    applySgrParameters(line.mid(i + 2, j - i - 2), currentStyle, baseStyle);
                i = j + 1;
                continue;
            }
        }

        // HTML-like tag
        if (parseHtml) {
            const int tagClose = parseHtmlTag(line, i, currentStyle, styleStack);
            if (tagClose != -1) {
                i = tagClose + 1;
                continue;
            }
        }

        // One grapheme cluster starting at i. The raw width drives layout;
        // control characters are written as their visible replacements.
        GraphemeIterator iterator(view.mid(i));
        GraphemeRef cluster;
        if (!iterator.next(cluster))
            break;
        visualWidth += appendGrapheme(cluster, currentStyle.fgColor, currentStyle.bgColor,
                                      currentStyle.attributes, entries, bytes);
        i += cluster.utf16Length();
    }
    return visualWidth;
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

int Text::appendGrapheme(const GraphemeRef &cluster, uint32_t fg, uint32_t bg,
                         TextAttribute attributes, std::vector<GlyphEntry> *entries,
                         QByteArray *bytes) {
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
        entry.fgColor = fg;
        entry.bgColor = bg;
        entry.attributes = attributes;
        entry.advance = static_cast<uint8_t>((std::min)(advance, 255));
        entry.cellWidth = static_cast<uint8_t>((std::min)(cellWidth, 255));
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

void Text::layoutSpans() const {
    // Segment the whole logical line so a style boundary never breaks a grapheme.
    QString content;
    QList<qsizetype> ends;
    ends.reserve(m_spans.size());
    for (const Span &span : m_spans) {
        content += span.text();
        ends.append(content.size());
    }
    const QStringView view(content);
    qsizetype spanIndex = 0;
    qsizetype lineStart = 0;
    const TextAttribute base = baseAttributes();
    do {
        qsizetype lineEnd = content.indexOf(u'\n', lineStart);
        if (lineEnd < 0) lineEnd = content.size();
        LineLayout line;
        line.firstGlyph = static_cast<int>(m_glyphs.size());
        GraphemeIterator iterator(view.mid(lineStart, lineEnd - lineStart));
        GraphemeRef cluster;
        qsizetype offset = lineStart;
        while (iterator.next(cluster)) {
            while (spanIndex + 1 < ends.size() && offset >= ends[spanIndex])
                ++spanIndex;
            const Span &span = m_spans[spanIndex];
            line.visualWidth += appendGrapheme(cluster,
                span.color().isValid() ? span.color().argb32() : m_resolvedColor,
                span.backgroundColor().isValid() ? span.backgroundColor().argb32() : 0,
                span.resolveAttributes(base), &m_glyphs, &m_glyphBytes);
            offset += cluster.utf16Length();
        }
        line.glyphCount = static_cast<int>(m_glyphs.size()) - line.firstGlyph;
        m_lines.push_back(line);
        lineStart = lineEnd + 1;
    } while (lineStart <= content.size());
}

void Text::ensureLayout() const {
    if (!m_layoutDirty) return;
    m_glyphs.clear();
    m_lines.clear();
    m_glyphBytes.clear();

    if (!m_spans.isEmpty()) {
        layoutSpans();
        m_layoutDirty = false;
        return;
    }

    const QStringList lines = m_text.split('\n');
    m_lines.reserve(static_cast<size_t>(lines.size()));
    for (const QString &line : lines) {
        LineLayout layout;
        layout.firstGlyph = static_cast<int>(m_glyphs.size());
        layout.visualWidth = parseLine(line, &m_glyphs, &m_glyphBytes);
        layout.glyphCount = static_cast<int>(m_glyphs.size()) - layout.firstGlyph;
        m_lines.push_back(layout);
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

    int currentY = absY + tp;
    if (m_verticalAlignment & Qt::AlignVCenter) {
        currentY += (availH - lineCount) / 2;
    } else if (m_verticalAlignment & Qt::AlignBottom) {
        currentY += (availH - lineCount);
    }

    const bool useGradient = (m_gradient && !m_gradient->stopsList().isEmpty());
    const int rightLimit = absX + width() - rp;
    const int leftLimit = absX + lp;
    const char *glyphBytes = m_glyphBytes.constData();

    for (const LineLayout &line : m_lines) {
        if (line.glyphCount > 0 && currentY >= absY + tp && currentY < absY + height() - bp && currentY >= 0 && currentY < buffer.height()) {
            const int visLen = line.visualWidth;
            int lineStartX = leftLimit;
            if (m_horizontalAlignment & Qt::AlignRight) {
                lineStartX += (availW - visLen);
            } else if (m_horizontalAlignment & Qt::AlignHCenter) {
                lineStartX += (availW - visLen) / 2;
            }

            int cx = lineStartX;
            int charIndexInLine = 0;
            const GlyphEntry *entry = m_glyphs.data() + line.firstGlyph;
            const GlyphEntry *end = entry + line.glyphCount;
            for (; entry != end; ++entry) {
                uint32_t charFgColor = entry->fgColor;
                if (useGradient) {
                    const qreal t = (visLen > 1) ? static_cast<qreal>(charIndexInLine) / (visLen - 1) : 0.0;
                    charFgColor = m_gradient->colorAt(t);
                }

                if (cx >= leftLimit && cx + entry->advance <= rightLimit &&
                    clipRect.contains(cx, currentY) &&
                    clipRect.contains(cx + (std::max)(1, int(entry->cellWidth)) - 1, currentY)) {
                    buffer.setGrapheme(cx, currentY,
                                       QByteArrayView(glyphBytes + entry->glyphOffset, entry->glyphLength),
                                       entry->cellWidth, charFgColor, entry->bgColor, entry->attributes
                                           | (!isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None));
                }
                cx += entry->advance;
                charIndexInLine += entry->advance;
            }
        }
        currentY++;
    }
}

} // namespace qamui
