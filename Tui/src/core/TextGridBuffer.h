#pragma once

#include "Cell.h"
#include <vector>
#include <string>

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
