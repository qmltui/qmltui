#include "Text.h"
#include "../core/ColorMapper.h"
#include <QRegularExpression>
#include <vector>

namespace qtui {

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

Text::Text(QObject *parent) : Item(parent) {
}

Text::~Text() = default;

void Text::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    
    QStringList lines = m_text.split('\n');
    int maxWidth = 0;
    for (const QString& line : lines) {
        int visLen = visualLength(line);
        if (visLen > maxWidth) maxWidth = visLen;
    }
    setImplicitWidth(maxWidth);
    setImplicitHeight(lines.count());
    
    emit textChanged();
    markDirty();
}

void Text::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void Text::setBold(bool bold) {
    if (m_bold == bold) return;
    m_bold = bold;
    emit boldChanged();
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
    
    // Re-evaluate implicit width and height
    QStringList lines = m_text.split('\n');
    int maxWidth = 0;
    for (const QString& line : lines) {
        int visLen = visualLength(line);
        if (visLen > maxWidth) maxWidth = visLen;
    }
    setImplicitWidth(maxWidth);
    
    emit textFormatChanged();
    markDirty();
}

int Text::visualLength(const QString &line) const {
    if (m_textFormat == PlainText) {
        return line.length();
    }
    int len = 0;
    TextStyle dummyStyle;
    std::vector<TextStyle> dummyStack;
    for (int i = 0; i < line.length(); ) {
        if ((m_textFormat == AutoText || m_textFormat == AnsiText) &&
            line[i] == '\033' && i + 1 < line.length() && line[i + 1] == '[') {
            int j = i + 2;
            while (j < line.length() && (line[j].isDigit() || line[j] == ';')) {
                j++;
            }
            if (j < line.length() && line[j] == 'm') {
                i = j + 1;
                continue;
            }
        }
        
        if (m_textFormat == AutoText || m_textFormat == RichText) {
            int tagClose = parseHtmlTag(line, i, dummyStyle, dummyStack);
            if (tagClose != -1) {
                i = tagClose + 1;
                continue;
            }
        }
        
        len++;
        i++;
    }
    return len;
}

void Text::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;

    Item::render(buffer, absX, absY, clipRect);

    QStringList lines = m_text.split('\n');
    
    int lp = leftPadding();
    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();

    int availW = width() - lp - rp;
    int availH = height() - tp - bp;

    int currentY = absY + tp;
    if (m_verticalAlignment & Qt::AlignVCenter) {
        currentY += (availH - lines.count()) / 2;
    } else if (m_verticalAlignment & Qt::AlignBottom) {
        currentY += (availH - lines.count());
    }

    // Base attributes and colors
    uint32_t baseFgColor = m_resolvedColor;
    uint32_t baseBgColor = 0x00000000;
    TextAttribute baseAttrs = TextAttribute::None;
    if (m_bold) baseAttrs = baseAttrs | TextAttribute::Bold;
    if (m_italic) baseAttrs = baseAttrs | TextAttribute::Italic;
    if (m_underline) baseAttrs = baseAttrs | TextAttribute::Underline;
    if (m_reversed) baseAttrs = baseAttrs | TextAttribute::Reversed;

    for (const QString& line : lines) {
        if (currentY >= absY + tp && currentY < absY + height() - bp && currentY >= 0 && currentY < buffer.height()) {
            int visLen = visualLength(line);
            int lineStartX = absX + lp;
            if (m_horizontalAlignment & Qt::AlignRight) {
                lineStartX += (availW - visLen);
            } else if (m_horizontalAlignment & Qt::AlignHCenter) {
                lineStartX += (availW - visLen) / 2;
            }

            int cx = lineStartX;
            TextStyle currentStyle{baseFgColor, baseBgColor, baseAttrs};
            std::vector<TextStyle> styleStack;

            for (int i = 0; i < line.length(); ) {
                bool skippedSequence = false;
                
                // SGR ANSI escape sequence parser: \033[...m
                if ((m_textFormat == AutoText || m_textFormat == AnsiText) &&
                    line[i] == '\033' && i + 1 < line.length() && line[i + 1] == '[') {
                    int j = i + 2;
                    QString paramsStr;
                    while (j < line.length() && (line[j].isDigit() || line[j] == ';')) {
                        paramsStr.append(line[j]);
                        j++;
                    }
                    if (j < line.length() && line[j] == 'm') {
                        QStringList params = paramsStr.split(';');
                        for (int pIdx = 0; pIdx < params.count(); ++pIdx) {
                            if (params[pIdx].isEmpty()) continue;
                            int code = params[pIdx].toInt();
                            if (code == 0) {
                                currentStyle.fgColor = baseFgColor;
                                currentStyle.bgColor = baseBgColor;
                                currentStyle.attributes = baseAttrs;
                            } else if (code == 1) {
                                currentStyle.attributes = currentStyle.attributes | TextAttribute::Bold;
                            } else if (code == 3) {
                                currentStyle.attributes = currentStyle.attributes | TextAttribute::Italic;
                            } else if (code == 4) {
                                currentStyle.attributes = currentStyle.attributes | TextAttribute::Underline;
                            } else if (code == 7) {
                                currentStyle.attributes = currentStyle.attributes | TextAttribute::Reversed;
                            } else if (code >= 30 && code <= 37) {
                                static const uint32_t ansiColors[] = {
                                    0xFF000000, 0xFFFF0000, 0xFF008000, 0xFFFFFF00,
                                    0xFF0000FF, 0xFFFF00FF, 0xFF00FFFF, 0xFFFFFFFF
                                };
                                currentStyle.fgColor = ansiColors[code - 30];
                            } else if (code >= 40 && code <= 47) {
                                static const uint32_t ansiColors[] = {
                                    0xFF000000, 0xFFFF0000, 0xFF008000, 0xFFFFFF00,
                                    0xFF0000FF, 0xFFFF00FF, 0xFF00FFFF, 0xFFFFFFFF
                                };
                                currentStyle.bgColor = ansiColors[code - 40];
                            } else if (code == 39) {
                                currentStyle.fgColor = baseFgColor;
                            } else if (code == 49) {
                                currentStyle.bgColor = baseBgColor;
                            } else if (code == 38 && pIdx + 4 < params.count() && params[pIdx + 1].toInt() == 2) {
                                int r = params[pIdx + 2].toInt();
                                int g = params[pIdx + 3].toInt();
                                int b = params[pIdx + 4].toInt();
                                currentStyle.fgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
                                pIdx += 4;
                            } else if (code == 48 && pIdx + 4 < params.count() && params[pIdx + 1].toInt() == 2) {
                                int r = params[pIdx + 2].toInt();
                                int g = params[pIdx + 3].toInt();
                                int b = params[pIdx + 4].toInt();
                                currentStyle.bgColor = 0xFF000000 | (r << 16) | (g << 8) | b;
                                pIdx += 4;
                            }
                        }
                        i = j + 1;
                        skippedSequence = true;
                    }
                }

                if (skippedSequence) continue;

                // HTML Tag parser
                if (m_textFormat == AutoText || m_textFormat == RichText) {
                    int tagClose = parseHtmlTag(line, i, currentStyle, styleStack);
                    if (tagClose != -1) {
                        i = tagClose + 1;
                        continue;
                    }
                }

                // Render character to cell
                if (cx >= absX + lp && cx < absX + width() - rp) {
                    if (clipRect.contains(cx, currentY)) {
                        if (cx >= 0 && cx < buffer.width()) {
                            Cell& cell = buffer.cellAt(cx, currentY);
                            QByteArray utf8 = line.mid(i, 1).toUtf8();
                            int len = std::min<int>(utf8.length(), 3);
                            for (int k = 0; k < len; ++k) {
                                cell.character[k] = utf8[k];
                            }
                            cell.character[len] = '\0';
                            cell.fgColor = currentStyle.fgColor;
                            cell.attributes = currentStyle.attributes;
                            if ((currentStyle.bgColor >> 24) != 0) {
                                cell.bgColor = currentStyle.bgColor;
                            }
                        }
                    }
                }
                cx++;
                i++;
            }
        }
        currentY++;
    }
}

} // namespace qtui
