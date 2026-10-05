#include "TextGridBuffer.h"
#include "UnicodeText.h"
#include <sstream>
#include <algorithm>
#include <limits>
#include <utility>

namespace qtui {

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
    Cell emptyCell;
    emptyCell.character = " ";
    std::fill(m_cells.begin(), m_cells.end(), emptyCell);
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

std::string TextGridBuffer::diff(const TextGridBuffer& nextBuffer) const {
    int curX = 0, curY = 0; // Assume cursor starts at 0,0 for relative diff
    return diffInternal(nextBuffer, true, curX, curY);
}

std::string TextGridBuffer::diffRelative(const TextGridBuffer& nextBuffer) const {
    int curX = 0, curY = 0; // Assume cursor starts at top-left of the region
    return diffInternal(nextBuffer, false, curX, curY);
}

std::string TextGridBuffer::diffInternal(const TextGridBuffer& nextBuffer, bool absolute, int& curX, int& curY) const {
    std::string ansi;
    ansi.reserve(static_cast<size_t>(nextBuffer.width()) *
                 static_cast<size_t>(nextBuffer.height()));

    bool cursorKnown = !absolute;
    bool styleKnown = false;
    uint32_t currentFg = 0;
    uint32_t currentBg = 0;
    uint8_t currentAttributes = 0;

    const auto appendCursorPosition = [&](int x, int y) {
        if (cursorKnown && curX == x && curY == y)
            return;

        if (absolute) {
            ansi += "\033[";
            ansi += std::to_string(y + 1);
            ansi += ';';
            ansi += std::to_string(x + 1);
            ansi += 'H';
        } else {
            const int dy = y - curY;
            const int dx = x - curX;
            if (dy != 0) {
                ansi += "\033[";
                ansi += std::to_string(std::abs(dy));
                ansi += dy > 0 ? 'B' : 'A';
            }
            if (dx != 0) {
                ansi += "\033[";
                ansi += std::to_string(std::abs(dx));
                ansi += dx > 0 ? 'C' : 'D';
            }
        }

        curX = x;
        curY = y;
        cursorKnown = true;
    };

    const auto normalizedColor = [](uint32_t color) {
        return (color >> 24) != 0 ? (0xFF000000u | (color & 0x00FFFFFFu)) : 0u;
    };

    const auto appendCellStyle = [&](const Cell& cell) {
        if (!styleKnown) {
            ansi += "\033[0m";
            styleKnown = true;
        }

        std::string parameters;
        const auto appendParameter = [&](const std::string& parameter) {
            if (!parameters.empty())
                parameters += ';';
            parameters += parameter;
        };

        const uint32_t nextFg = normalizedColor(cell.fgColor);
        if (nextFg != currentFg) {
            if (nextFg == 0) {
                appendParameter("39");
            } else {
                appendParameter("38;2;" + std::to_string((nextFg >> 16) & 0xFF) + ";" +
                                std::to_string((nextFg >> 8) & 0xFF) + ";" +
                                std::to_string(nextFg & 0xFF));
            }
            currentFg = nextFg;
        }

        const uint32_t nextBg = normalizedColor(cell.bgColor);
        if (nextBg != currentBg) {
            if (nextBg == 0) {
                appendParameter("49");
            } else {
                appendParameter("48;2;" + std::to_string((nextBg >> 16) & 0xFF) + ";" +
                                std::to_string((nextBg >> 8) & 0xFF) + ";" +
                                std::to_string(nextBg & 0xFF));
            }
            currentBg = nextBg;
        }

        const uint8_t nextAttributes = static_cast<uint8_t>(cell.attributes);
        const uint8_t removedAttributes = currentAttributes & ~nextAttributes;
        const uint8_t addedAttributes = nextAttributes & ~currentAttributes;
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Bold))
            appendParameter("22");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Italic))
            appendParameter("23");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Underline))
            appendParameter("24");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Reversed))
            appendParameter("27");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Blink))
            appendParameter("25");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Conceal))
            appendParameter("28");
        if (removedAttributes & static_cast<uint8_t>(TextAttribute::Strikethrough))
            appendParameter("29");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Bold))
            appendParameter("1");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Italic))
            appendParameter("3");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Underline))
            appendParameter("4");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Reversed))
            appendParameter("7");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Blink))
            appendParameter("5");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Conceal))
            appendParameter("8");
        if (addedAttributes & static_cast<uint8_t>(TextAttribute::Strikethrough))
            appendParameter("9");
        currentAttributes = nextAttributes;

        if (!parameters.empty()) {
            ansi += "\033[";
            ansi += parameters;
            ansi += 'm';
        }
    };

    for (int y = 0; y < nextBuffer.height(); ++y) {
        int x = 0;
        while (x < nextBuffer.width()) {
            const auto cellChanged = [&](int column) {
                return column >= m_width || y >= m_height ||
                       nextBuffer.cellAt(column, y) != cellAt(column, y);
            };

            while (x < nextBuffer.width() && !cellChanged(x))
                ++x;
            if (x >= nextBuffer.width())
                break;

            int runEnd = x + 1;
            while (runEnd < nextBuffer.width() && cellChanged(runEnd))
                ++runEnd;

            for (int column = x; column < runEnd; ++column) {
                const Cell& nextCell = nextBuffer.cellAt(column, y);
                if (nextCell.continuation)
                    continue;

                appendCursorPosition(column, y);
                appendCellStyle(nextCell);
                ansi += nextCell.character.isEmpty()
                    ? std::string(" ") : nextCell.character.toStdString();
                curX = column + std::max(1, static_cast<int>(nextCell.displayWidth));
                curY = y;
            }

            x = runEnd;
        }
    }
    return ansi;
}

std::string TextGridBuffer::toAnsiString() const {
    std::ostringstream ansi;
    uint32_t lastFg = 0;
    uint32_t lastBg = 0;
    uint8_t lastAttr = 0;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Cell& cell = cellAt(x, y);
            
            bool colorChanged = false;
            if (cell.fgColor != lastFg) { lastFg = cell.fgColor; colorChanged = true; }
            if (cell.bgColor != lastBg) { lastBg = cell.bgColor; colorChanged = true; }
            if (cell.attributes != static_cast<TextAttribute>(lastAttr)) { lastAttr = static_cast<uint8_t>(cell.attributes); colorChanged = true; }
            
            if (colorChanged) {
                ansi << "\033[0m"; // Reset first to be safe
                if ((lastFg >> 24) != 0) {
                    ansi << "\033[38;2;" << ((lastFg >> 16) & 0xFF) << ";" << ((lastFg >> 8) & 0xFF) << ";" << (lastFg & 0xFF) << "m";
                }
                if ((lastBg >> 24) != 0) {
                    ansi << "\033[48;2;" << ((lastBg >> 16) & 0xFF) << ";" << ((lastBg >> 8) & 0xFF) << ";" << (lastBg & 0xFF) << "m";
                }
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Bold))      ansi << "\033[1m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Italic))    ansi << "\033[3m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Underline)) ansi << "\033[4m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Reversed))  ansi << "\033[7m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Blink)) ansi << "\033[5m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Conceal)) ansi << "\033[8m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Strikethrough)) ansi << "\033[9m";
            }
            
            if (!cell.continuation)
                ansi << (cell.character.isEmpty() ? std::string(" ") : cell.character.toStdString());
        }
        ansi << "\033[0m\n"; // Reset at end of line and add newline
        lastFg = 0; lastBg = 0; lastAttr = 0;
    }
    
    // Remove the very last newline if we want it completely flush, but standard logs have \n
    std::string result = ansi.str();
    if (!result.empty() && result.back() == '\n') {
        result.pop_back(); // Remove the trailing newline since print() usually appends it if needed, or we just keep it?
        // Actually print() appends \n if it doesn't end with one. So let's leave it without the trailing newline,
        // to avoid double newlines.
    }
    return result;
}

void TextGridBuffer::writeText(int x, int y, const QString& text, uint32_t fgColor, uint32_t bgColor, TextAttribute attributes) {
    if (y < 0 || y >= m_height || x >= m_width) return;

    int curX = x;
    QString safeText;
    safeText.reserve(text.size());
    for (char32_t codepoint : text.toUcs4()) {
        if (codepoint <= 0x1f) {
            safeText.append(QChar(static_cast<char16_t>(0x2400 + codepoint)));
        } else if (codepoint == 0x7f) {
            safeText.append(QChar(0x2421));
        } else if (codepoint >= 0x80 && codepoint <= 0x9f) {
            safeText.append(QChar(0xfffd));
        } else {
            safeText.append(QString::fromUcs4(&codepoint, 1));
        }
    }

    for (const Grapheme &cluster : UnicodeText::graphemes(safeText)) {
        const int width = std::max(0, cluster.width);
        if (width == 0 && curX > 0) {
            int targetX = curX - 1;
            while (targetX > 0 && cellAt(targetX, y).continuation)
                --targetX;
            cellAt(targetX, y).character += cluster.utf8;
            continue;
        }
        if (width > 0 && curX + width > m_width)
            break;
        if (curX >= 0 && curX < m_width) {
            Cell& cell = cellAt(curX, y);
            cell.fgColor = fgColor;
            if ((bgColor >> 24) != 0)
                cell.bgColor = bgColor;
            cell.attributes = attributes;
            cell.character = cluster.utf8;
            cell.displayWidth = static_cast<uint8_t>(width);
            cell.continuation = false;
            for (int offset = 1; offset < width; ++offset) {
                Cell &continuation = cellAt(curX + offset, y);
                const uint32_t underlyingBg = continuation.bgColor;
                continuation.fgColor = cell.fgColor;
                continuation.bgColor = (bgColor >> 24) != 0 ? bgColor : underlyingBg;
                continuation.attributes = cell.attributes;
                continuation.character.clear();
                continuation.displayWidth = 0;
                continuation.continuation = true;
            }
        }
        curX += width;
    }
}

void TextGridBuffer::drawLine(int x1, int y1, int x2, int y2, const char* character, uint32_t fgColor, uint32_t bgColor) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    auto setChar = [&](int cx, int cy) {
        if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
            Cell& cell = cellAt(cx, cy);
            cell.fgColor = fgColor;
            cell.bgColor = bgColor;
            cell.character = QByteArray(character);
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
    for (int cy = y; cy < y + height; ++cy) {
        for (int cx = x; cx < x + width; ++cx) {
            if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
                Cell& cell = cellAt(cx, cy);
                cell.fgColor = fgColor;
                cell.bgColor = bgColor;
                cell.character = QByteArray(character);
                cell.displayWidth = 1;
                cell.continuation = false;
            }
        }
    }
}

void TextGridBuffer::drawBox(int x, int y, int width, int height, uint32_t fgColor, uint32_t bgColor) {
    if (width <= 0 || height <= 0) return;

    const char* topLeft     = "\xE2\x94\x8C"; // ┌
    const char* topRight    = "\xE2\x94\x90"; // ┐
    const char* bottomLeft  = "\xE2\x94\x94"; // └
    const char* bottomRight = "\xE2\x94\x98"; // ┘
    const char* horizontal  = "\xE2\x94\x80"; // ─
    const char* vertical    = "\xE2\x94\x82"; // │

    auto setCellChar = [&](int cx, int cy, const char* ch) {
        if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
            Cell& cell = cellAt(cx, cy);
            cell.fgColor = fgColor;
            cell.bgColor = bgColor;
            cell.character = QByteArray(ch);
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

} // namespace qtui
