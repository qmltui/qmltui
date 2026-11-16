#include "BigText.h"
#include <QmlTui/Core/Primitives/Color.h>
#include <QChar>
#include <array>
#include <cctype>

namespace qmltui {

// 3x3 3-line tall ASCII block font matrix definitions
struct FontGlyph {
    char ch;
    std::array<const char*, 3> lines;
};

static const FontGlyph g_bigFont[] = {
    {'0', {"█▀█", "█ █", "▀▀▀"}}, {'1', {" █ ", " █ ", " ▀ "}}, {'2', {"▀▀█", "█▀▀", "▀▀▀"}},
    {'3', {"▀▀█", " ▀█", "▀▀▀"}}, {'4', {"█ █", "▀▀█", "  ▀"}}, {'5', {"█▀▀", "▀▀█", "▀▀▀"}},
    {'6', {"█▀▀", "█▀█", "▀▀▀"}}, {'7', {"▀▀█", "  █", "  ▀"}}, {'8', {"█▀█", "█▀█", "▀▀▀"}},
    {'9', {"█▀█", "▀▀█", "▀▀▀"}}, {':', {"   ", " ▀ ", " ▀ "}}, {'-', {"   ", "▀▀▀", "   "}},
    {'+', {" █ ", "▀█▀", " ▀ "}}, {'/', {"  █", " █ ", "█  "}}, {'=', {"▀▀▀", "   ", "▀▀▀"}},
    {'A', {"█▀█", "█▀█", "▀ ▀"}}, {'B', {"█▀█", "█▀▀", "▀▀▀"}}, {'C', {"█▀▀", "█  ", "▀▀▀"}},
    {'D', {"█▀▄", "█ █", "▀▀ "}}, {'E', {"█▀▀", "█▀▀", "▀▀▀"}}, {'F', {"█▀▀", "█▀▀", "▀  "}},
    {'G', {"█▀▀", "█ █", "▀▀▀"}}, {'H', {"█ █", "█▀█", "▀ ▀"}}, {'I', {"▀█▀", " █ ", "▀▀▀"}},
    {'J', {"  █", "  █", "▀▀ "}}, {'K', {"█ █", "█▀ ", "▀ ▀"}}, {'L', {"█  ", "█  ", "▀▀▀"}},
    {'M', {"█/█", "█ █", "▀ ▀"}}, {'N', {"█▀█", "█ █", "▀ ▀"}}, {'O', {"█▀█", "█ █", "▀▀▀"}},
    {'P', {"█▀█", "█▀▀", "▀  "}}, {'Q', {"█▀█", "█ █", "▀▀█"}}, {'R', {"█▀█", "█▀ ", "▀ ▀"}},
    {'S', {"█▀▀", "▀▀█", "▀▀▀"}}, {'T', {"▀█▀", " █ ", " ▀ "}}, {'U', {"█ █", "█ █", "▀▀▀"}},
    {'V', {"█ █", "█ █", " ▀ "}}, {'W', {"█ █", "█/█", " ▀ "}}, {'X', {"█ █", " █ ", "▀ ▀"}},
    {'Y', {"█ █", " █ ", " ▀ "}}, {'Z', {"▀▀█", " █ ", "▀▀▀"}}, {' ', {"   ", "   ", "   "}}};

static const FontGlyph* findGlyph(char c) {
    char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (const auto& g : g_bigFont) {
        if (g.ch == upper) return &g;
    }
    return &g_bigFont[41]; // Space
}

BigText::BigText(QObject* parent) : Item(parent) {
    setImplicitWidth(12);
    setImplicitHeight(3);
}

void BigText::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    setImplicitWidth(m_text.length() * 4);
    emit textChanged();
    markDirty();
}

void BigText::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::resolve(m_color, Color::DefaultTag);
    emit colorChanged();
    markDirty();
}

void BigText::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor = Color::resolve(m_backgroundColor, Color::TransparentTag);
    emit backgroundColorChanged();
    markDirty();
}

void BigText::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible() || m_text.isEmpty()) return;

    const ClipRect effectiveClip =
        clipRect.intersected(ClipRect{absX, absY, width(), height()})
            .intersected(ClipRect{0, 0, buffer.width(), buffer.height()});
    if (effectiveClip.isEmpty()) return;

    const bool paintBg = !Color::isTransparent(m_resolvedBackgroundColor);
    const uint32_t bg = Color::paintedArgb32(m_resolvedBackgroundColor);

    int curX = absX;
    for (qsizetype idx = 0; idx < m_text.size(); ++idx) {
        if (curX >= effectiveClip.x + effectiveClip.width) break;
        if (curX + 3 > effectiveClip.x) {
            const FontGlyph* glyph = findGlyph(m_text[idx].toLatin1());
            for (int row = 0; row < 3; ++row) {
                const int cellY = absY + row;
                if (cellY < effectiveClip.y || cellY >= effectiveClip.y + effectiveClip.height)
                    continue;
                const char* p = glyph->lines[row];
                for (int col = 0; col < 3 && *p != '\0'; ++col) {
                    const unsigned char lead = static_cast<unsigned char>(*p);
                    const int seqLen = (lead < 0x80U)   ? 1
                                       : (lead < 0xE0U) ? 2
                                       : (lead < 0xF0U) ? 3
                                                        : 4;
                    const int cellX = curX + col;
                    if (cellX >= effectiveClip.x && cellX < effectiveClip.x + effectiveClip.width) {
                        buffer.setGrapheme(cellX, cellY, QByteArrayView(p, seqLen), 1,
                                           m_resolvedColor, bg);
                        if (paintBg && bg == Color::DefaultTag) {
                            buffer.cellAt(cellX, cellY).bgColor = Color::DefaultTag;
                        }
                    }
                    p += seqLen;
                }
            }
        }
        curX += 4; // 3 columns wide + 1 column spacing
    }
}

} // namespace qmltui
