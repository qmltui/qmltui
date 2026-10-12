#include <Qamui/Core/primitives/TextGridBuffer.h>
#include <Qamui/Core/primitives/UnicodeText.h>
#include <algorithm>
#include <limits>
#include <utility>

namespace qamui {

TextGridBuffer::TextGridBuffer() = default;

bool TextGridBuffer::resize(int width, int height) {
    constexpr size_t MaxCellCount = 10'000'000;
    if (width < 0 || height < 0) return false;
    const size_t cellWidth = static_cast<size_t>(width);
    const size_t cellHeight = static_cast<size_t>(height);
    if (cellHeight != 0 && cellWidth > MaxCellCount / cellHeight)
        return false;
    const size_t cellCount = cellWidth * cellHeight;
    if (cellCount > MaxCellCount) return false;

    m_width = width;
    m_height = height;
    m_cells.resize(cellCount);
    clear();
    return true;
}

void TextGridBuffer::swap(TextGridBuffer& other) noexcept {
    using std::swap;
    swap(m_width, other.m_width);
    swap(m_height, other.m_height);
    swap(m_cells, other.m_cells);
}

void TextGridBuffer::clear() {
    std::fill(m_cells.begin(), m_cells.end(), Cell{});
}

bool TextGridBuffer::contains(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

Cell *TextGridBuffer::tryCellAt(int x, int y)
{
    return contains(x, y) ? &m_cells[static_cast<size_t>(y * m_width + x)] : nullptr;
}

const Cell *TextGridBuffer::tryCellAt(int x, int y) const
{
    return contains(x, y) ? &m_cells[static_cast<size_t>(y * m_width + x)] : nullptr;
}

Cell& TextGridBuffer::cellAt(int x, int y) {
    Q_ASSERT(contains(x, y));
    return m_cells[static_cast<size_t>(y * m_width + x)];
}

const Cell& TextGridBuffer::cellAt(int x, int y) const {
    Q_ASSERT(contains(x, y));
    return m_cells[static_cast<size_t>(y * m_width + x)];
}

// diff(), diffRelative() and toAnsiString() live in TextGridBufferAnsi.cpp.

void TextGridBuffer::writeText(int x, int y, const QString& text, uint32_t fgColor, uint32_t bgColor, TextAttribute attributes) {
    if (y < 0 || y >= m_height || x >= m_width) return;

    int curX = x;
    GraphemeIterator iterator(text, /*sanitizeControls=*/true);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        const int advance = setGrapheme(curX, y, cluster.utf16, cluster.width,
                                        fgColor, bgColor, attributes);
        if (advance < 0)
            break;
        curX += advance;
    }
}

int TextGridBuffer::setGrapheme(int x, int y, QStringView cluster, int width,
                                uint32_t fgColor, uint32_t bgColor, TextAttribute attributes)
{
    char encoded[64];
    const int length = UnicodeText::encodeUtf8(cluster, encoded, sizeof encoded,
                                               /*sanitizeControls=*/true);
    if (length >= 0)
        return setGrapheme(x, y, QByteArrayView(encoded, length), width, fgColor, bgColor, attributes);
    const QByteArray fallback = UnicodeText::toUtf8(cluster, /*sanitizeControls=*/true);
    return setGrapheme(x, y, QByteArrayView(fallback), width, fgColor, bgColor, attributes);
}

int TextGridBuffer::setGrapheme(int x, int y, QByteArrayView utf8, int width,
                                uint32_t fgColor, uint32_t bgColor, TextAttribute attributes)
{
    width = (std::max)(0, width);
    if (m_width <= 0 || m_height <= 0 || y < 0 || y >= m_height)
        return width;

    if (width == 0 && x > 0 && m_width > 0) {
        int targetX = (std::min)(x, m_width) - 1;
        while (targetX > 0 && cellAt(targetX, y).continuation)
            --targetX;
        cellAt(targetX, y).appendCharacter(utf8);
        return 0;
    }
    if (width > 0 && x + width > m_width)
        return -1;
    if (x < 0 || x >= m_width)
        return width;

    Cell& cell = cellAt(x, y);
    cell.fgColor = fgColor;
    if ((bgColor >> 24) != 0)
        cell.bgColor = bgColor;
    cell.attributes = attributes;
    cell.setCharacter(utf8);
    cell.displayWidth = static_cast<uint8_t>(width);
    cell.continuation = false;
    for (int offset = 1; offset < width; ++offset) {
        Cell &continuation = cellAt(x + offset, y);
        if ((bgColor >> 24) != 0)
            continuation.bgColor = bgColor;
        continuation.fgColor = fgColor;
        continuation.attributes = attributes;
        continuation.clearCharacter();
        continuation.displayWidth = 0;
        continuation.continuation = true;
    }
    return width;
}

void TextGridBuffer::drawLine(int x1, int y1, int x2, int y2, const char* character, uint32_t fgColor, uint32_t bgColor) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    const QByteArrayView glyph(character);   // measured once, not per cell
    auto setChar = [&](int cx, int cy) {
        if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
            Cell& cell = cellAt(cx, cy);
            cell.fgColor = fgColor;
            cell.bgColor = bgColor;
            cell.setCharacter(glyph);
            cell.displayWidth = 1;
            cell.continuation = false;
        }
    };

    while (true) {
        setChar(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void TextGridBuffer::fillRect(int x, int y, int width, int height, const char* character, uint32_t fgColor, uint32_t bgColor) {
    const QByteArrayView glyph(character);   // measured once, not per cell
    const int startX = (std::max)(x, 0);
    const int endX = (std::min)(x + width, m_width);
    const int startY = (std::max)(y, 0);
    const int endY = (std::min)(y + height, m_height);
    for (int cy = startY; cy < endY; ++cy) {
        for (int cx = startX; cx < endX; ++cx) {
            Cell& cell = cellAt(cx, cy);
            cell.fgColor = fgColor;
            cell.bgColor = bgColor;
            cell.setCharacter(glyph);
            cell.displayWidth = 1;
            cell.continuation = false;
        }
    }
}

void TextGridBuffer::drawBox(int x, int y, int width, int height, uint32_t fgColor, uint32_t bgColor) {
    if (width <= 0 || height <= 0) return;

    const QByteArrayView topLeft("\xE2\x94\x8C"); // ┌
    const QByteArrayView topRight("\xE2\x94\x90"); // ┐
    const QByteArrayView bottomLeft("\xE2\x94\x94"); // └
    const QByteArrayView bottomRight("\xE2\x94\x98"); // ┘
    const QByteArrayView horizontal("\xE2\x94\x80"); // ─
    const QByteArrayView vertical("\xE2\x94\x82"); // │

    auto setCellChar = [&](int cx, int cy, QByteArrayView glyph) {
        if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
            Cell& cell = cellAt(cx, cy);
            cell.fgColor = fgColor;
            cell.bgColor = bgColor;
            cell.setCharacter(glyph);
            cell.displayWidth = 1;
            cell.continuation = false;
        }
    };

    if (width == 1 && height == 1) {
        setCellChar(x, y, topLeft);
        return;
    }

    for (int cx = x + 1; cx < x + width - 1; ++cx) {
        setCellChar(cx, y, horizontal);
        if (height > 1) {
            setCellChar(cx, y + height - 1, horizontal);
        }
    }

    for (int cy = y + 1; cy < y + height - 1; ++cy) {
        setCellChar(x, cy, vertical);
        if (width > 1) {
            setCellChar(x + width - 1, cy, vertical);
        }
    }

    setCellChar(x, y, topLeft);
    if (width > 1) setCellChar(x + width - 1, y, topRight);
    if (height > 1) setCellChar(x, y + height - 1, bottomLeft);
    if (width > 1 && height > 1) setCellChar(x + width - 1, y + height - 1, bottomRight);
}

} // namespace qamui
