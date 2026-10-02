#pragma once

#include "Cell.h"
#include <vector>
#include <string>
#include <QString>

namespace qtui {

class TextGridBuffer {
public:
    TextGridBuffer();

    void resize(int width, int height);
    void clear();

    int width() const { return m_width; }
    int height() const { return m_height; }

    Cell& cellAt(int x, int y);
    const Cell& cellAt(int x, int y) const;

    // Drawing and text helpers
    void writeText(int x, int y, const QString& text, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    void drawLine(int x1, int y1, int x2, int y2, const char* character = "*", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);
    void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);
    void fillRect(int x, int y, int width, int height, const char* character = " ", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);

    // Generates ANSI escape sequences using absolute cursor positioning.
    std::string diff(const TextGridBuffer& nextBuffer) const;

    // Generates ANSI escape sequences using relative cursor movements.
    std::string diffRelative(const TextGridBuffer& nextBuffer) const;

    // Converts the buffer linearly to an ANSI string with newlines.
    std::string toAnsiString() const;

private:
    std::string diffInternal(const TextGridBuffer& nextBuffer, bool absolute, int& curX, int& curY) const;

    int m_width = 0;
    int m_height = 0;
    std::vector<Cell> m_cells;
    Cell m_emptyCell;
};

} // namespace qtui
