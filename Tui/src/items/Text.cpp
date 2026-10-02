#include "Text.h"
#include "../core/ColorMapper.h"

namespace qtui {

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

int Text::visualLength(const QString &line) const {
    int len = 0;
    for (int i = 0; i < line.length(); ) {
        if (line[i] == '\033' && i + 1 < line.length() && line[i + 1] == '[') {
            int j = i + 2;
            while (j < line.length() && (line[j].isDigit() || line[j] == ';')) {
                j++;
            }
            if (j < line.length() && line[j] == 'm') {
                i = j + 1;
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
    uint32_t baseBgColor = m_resolvedBgColor;
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
            uint32_t currentFg = baseFgColor;
            uint32_t currentBg = baseBgColor;
            TextAttribute currentAttrs = baseAttrs;

            for (int i = 0; i < line.length(); ) {
                // SGR ANSI escape sequence parser: \033[...m
                if (line[i] == '\033' && i + 1 < line.length() && line[i + 1] == '[') {
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
                                currentFg = baseFgColor;
                                currentBg = baseBgColor;
                                currentAttrs = baseAttrs;
                            } else if (code == 1) {
                                currentAttrs = currentAttrs | TextAttribute::Bold;
                            } else if (code == 3) {
                                currentAttrs = currentAttrs | TextAttribute::Italic;
                            } else if (code == 4) {
                                currentAttrs = currentAttrs | TextAttribute::Underline;
                            } else if (code == 7) {
                                currentAttrs = currentAttrs | TextAttribute::Reversed;
                            } else if (code >= 30 && code <= 37) {
                                static const uint32_t ansiColors[] = {
                                    0xFF000000, 0xFFFF0000, 0xFF008000, 0xFFFFFF00,
                                    0xFF0000FF, 0xFFFF00FF, 0xFF00FFFF, 0xFFFFFFFF
                                };
                                currentFg = ansiColors[code - 30];
                            } else if (code >= 40 && code <= 47) {
                                static const uint32_t ansiColors[] = {
                                    0xFF000000, 0xFFFF0000, 0xFF008000, 0xFFFFFF00,
                                    0xFF0000FF, 0xFFFF00FF, 0xFF00FFFF, 0xFFFFFFFF
                                };
                                currentBg = ansiColors[code - 40];
                            } else if (code == 39) {
                                currentFg = baseFgColor;
                            } else if (code == 49) {
                                currentBg = baseBgColor;
                            } else if (code == 38 && pIdx + 4 < params.count() && params[pIdx + 1].toInt() == 2) {
                                int r = params[pIdx + 2].toInt();
                                int g = params[pIdx + 3].toInt();
                                int b = params[pIdx + 4].toInt();
                                currentFg = 0xFF000000 | (r << 16) | (g << 8) | b;
                                pIdx += 4;
                            } else if (code == 48 && pIdx + 4 < params.count() && params[pIdx + 1].toInt() == 2) {
                                int r = params[pIdx + 2].toInt();
                                int g = params[pIdx + 3].toInt();
                                int b = params[pIdx + 4].toInt();
                                currentBg = 0xFF000000 | (r << 16) | (g << 8) | b;
                                pIdx += 4;
                            }
                        }
                        i = j + 1;
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
                            cell.fgColor = currentFg;
                            cell.attributes = currentAttrs;
                            if ((currentBg >> 24) != 0) {
                                cell.bgColor = currentBg;
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

    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
