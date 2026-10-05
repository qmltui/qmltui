#pragma once

#include "Cell.h"
#include <vector>
#include <string>
#include <QString>

namespace qtui {

class TextGridBuffer {
public:
    TextGridBuffer();

    bool resize(int width, int height);
    void clear();
    void swap(TextGridBuffer& other) noexcept;

    int width() const { return m_width; }
    int height() const { return m_height; }

    bool contains(int x, int y) const;
    Cell *tryCellAt(int x, int y);
    const Cell *tryCellAt(int x, int y) const;

    Cell& cellAt(int x, int y);
    const Cell& cellAt(int x, int y) const;

    // Drawing and text helpers
    void writeText(int x, int y, const QString& text, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    void writeText(int x, int y, const QString &text, Color fgColor,
                   Color bgColor = Color::fromArgb32(0x00000000),
                   TextAttribute attributes = TextAttribute::None)
    {
        writeText(x, y, text, fgColor.argb32(), bgColor.argb32(), attributes);
    }
    void drawLine(int x1, int y1, int x2, int y2, const char* character = "*", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);
    void drawLine(int x1, int y1, int x2, int y2, const char *character,
                  Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000))
    {
        drawLine(x1, y1, x2, y2, character, fgColor.argb32(), bgColor.argb32());
    }
    void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);
    void drawBox(int x, int y, int width, int height, Color fgColor,
                 Color bgColor = Color::fromArgb32(0x00000000))
    {
        drawBox(x, y, width, height, fgColor.argb32(), bgColor.argb32());
    }
    void fillRect(int x, int y, int width, int height, const char* character = " ", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000);
    void fillRect(int x, int y, int width, int height, const char *character,
                  Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000))
    {
        fillRect(x, y, width, height, character, fgColor.argb32(), bgColor.argb32());
    }

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
};

} // namespace qtui
