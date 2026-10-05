#include "BigText.h"
#include <ColorMapper.h>
#include <QChar>
#include <array>
#include <cctype>

namespace qtui {

// 3x3 3-line tall ASCII block font matrix definitions
struct FontGlyph {
    char ch;
    std::array<const char*, 3> lines;
};

static const FontGlyph g_bigFont[] = {
    { '0', { "█▀█", "█ █", "▀▀▀" } },
    { '1', { " █ ", " █ ", " ▀ " } },
    { '2', { "▀▀█", "█▀▀", "▀▀▀" } },
    { '3', { "▀▀█", " ▀█", "▀▀▀" } },
    { '4', { "█ █", "▀▀█", "  ▀" } },
    { '5', { "█▀▀", "▀▀█", "▀▀▀" } },
    { '6', { "█▀▀", "█▀█", "▀▀▀" } },
    { '7', { "▀▀█", "  █", "  ▀" } },
    { '8', { "█▀█", "█▀█", "▀▀▀" } },
    { '9', { "█▀█", "▀▀█", "▀▀▀" } },
    { ':', { "   ", " ▀ ", " ▀ " } },
    { '-', { "   ", "▀▀▀", "   " } },
    { '+', { " █ ", "▀█▀", " ▀ " } },
    { '/', { "  █", " █ ", "█  " } },
    { '=', { "▀▀▀", "   ", "▀▀▀" } },
    { 'A', { "█▀█", "█▀█", "▀ ▀" } },
    { 'B', { "█▀█", "█▀▀", "▀▀▀" } },
    { 'C', { "█▀▀", "█  ", "▀▀▀" } },
    { 'D', { "█▀▄", "█ █", "▀▀ " } },
    { 'E', { "█▀▀", "█▀▀", "▀▀▀" } },
    { 'F', { "█▀▀", "█▀▀", "▀  " } },
    { 'G', { "█▀▀", "█ █", "▀▀▀" } },
    { 'H', { "█ █", "█▀█", "▀ ▀" } },
    { 'I', { "▀█▀", " █ ", "▀▀▀" } },
    { 'J', { "  █", "  █", "▀▀ " } },
    { 'K', { "█ █", "█▀ ", "▀ ▀" } },
    { 'L', { "█  ", "█  ", "▀▀▀" } },
    { 'M', { "█/█", "█ █", "▀ ▀" } },
    { 'N', { "█▀█", "█ █", "▀ ▀" } },
    { 'O', { "█▀█", "█ █", "▀▀▀" } },
    { 'P', { "█▀█", "█▀▀", "▀  " } },
    { 'Q', { "█▀█", "█ █", "▀▀█" } },
    { 'R', { "█▀█", "█▀ ", "▀ ▀" } },
    { 'S', { "█▀▀", "▀▀█", "▀▀▀" } },
    { 'T', { "▀█▀", " █ ", " ▀ " } },
    { 'U', { "█ █", "█ █", "▀▀▀" } },
    { 'V', { "█ █", "█ █", " ▀ " } },
    { 'W', { "█ █", "█/█", " ▀ " } },
    { 'X', { "█ █", " █ ", "▀ ▀" } },
    { 'Y', { "█ █", " █ ", " ▀ " } },
    { 'Z', { "▀▀█", " █ ", "▀▀▀" } },
    { ' ', { "   ", "   ", "   " } }
};

static const FontGlyph* findGlyph(char c) {
    char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (const auto &g : g_bigFont) {
        if (g.ch == upper) return &g;
    }
    return &g_bigFont[41]; // Space
}

BigText::BigText(QObject *parent) : Item(parent) {
    setImplicitWidth(12);
    setImplicitHeight(3);
}

void BigText::setText(const QString &text) {
    if (m_text == text) return;
    m_text = text;
    setImplicitWidth(m_text.length() * 4);
    emit textChanged();
    markDirty();
}

void BigText::setColor(const QString &color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFF00);
    emit colorChanged();
    markDirty();
}

void BigText::setBgColor(const QString &color) {
    if (m_bgColor == color) return;
    m_bgColor = color;
    m_resolvedBgColor = ColorMapper::fromString(m_bgColor, 0x00000000);
    emit bgColorChanged();
    markDirty();
}

void BigText::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible || m_text.isEmpty()) return;

    Item::render(buffer, absX, absY, clipRect);

    const int startX = absX;
    const int startY = absY;

    int curX = startX;
    for (int idx = 0; idx < m_text.length(); ++idx) {
        char c = m_text[idx].toLatin1();
        const FontGlyph* glyph = findGlyph(c);

        for (int row = 0; row < 3; ++row) {
            buffer.writeText(curX, startY + row, QString::fromUtf8(glyph->lines[row]), m_resolvedColor, m_resolvedBgColor);
        }
        curX += 4; // 3 chars wide + 1 space spacing
    }
}

} // namespace qtui
