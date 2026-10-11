#pragma once

#include <Qamui/Core/primitives/Cell.h>
#include <Qamui/Core/primitives/TerminalCapabilities.h>
#include <vector>
#include <string>
#include <QString>

namespace qamui {

// How a frame diff is encoded for the terminal.
struct AnsiEncodeOptions {
    ColorDepth colorDepth = ColorDepth::TrueColor;
    bool eraseToEndOfLine = true;   // "\033[K" instead of a run of trailing blanks
    int eraseMinimumCells = 4;      // only when at least this many blanks remain
};

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

    // Writes one already-segmented grapheme cluster of the given display width
    // at (x, y): the base cell plus width-1 continuation cells. A background
    // with zero alpha keeps the underlying background. A zero-width cluster at
    // x > 0 is appended to the nearest base cell on the left.
    //
    // Returns the number of columns to advance (width, or 0 for an appended
    // zero-width cluster), or -1 when the cluster does not fit before the right
    // edge and nothing was written. Rows outside the buffer are ignored but
    // still advance.
    int setGrapheme(int x, int y, QByteArrayView utf8, int width,
                    uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000,
                    TextAttribute attributes = TextAttribute::None);
    int setGrapheme(int x, int y, QByteArrayView utf8, int width, Color fgColor,
                    Color bgColor = Color::fromArgb32(0x00000000),
                    TextAttribute attributes = TextAttribute::None)
    {
        return setGrapheme(x, y, utf8, width, fgColor.argb32(), bgColor.argb32(), attributes);
    }

    // Same as above for a UTF-16 cluster (typically a GraphemeRef::utf16 view);
    // control characters are replaced by their visible glyphs as in writeText.
    int setGrapheme(int x, int y, QStringView cluster, int width,
                    uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000,
                    TextAttribute attributes = TextAttribute::None);
    int setGrapheme(int x, int y, QStringView cluster, int width, Color fgColor,
                    Color bgColor = Color::fromArgb32(0x00000000),
                    TextAttribute attributes = TextAttribute::None)
    {
        return setGrapheme(x, y, cluster, width, fgColor.argb32(), bgColor.argb32(), attributes);
    }

    // Drawing and text helpers. Omitted foregrounds use the terminal default;
    // zero-alpha backgrounds preserve the surface underneath.
    void writeText(int x, int y, const QString& text, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    void writeText(int x, int y, const QString &text, Color fgColor,
                   Color bgColor = Color::fromArgb32(0x00000000),
                   TextAttribute attributes = TextAttribute::None)
    {
        writeText(x, y, text, fgColor.argb32(), bgColor.argb32(), attributes);
    }
    void drawLine(int x1, int y1, int x2, int y2, const char* character = "*", uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000);
    void drawLine(int x1, int y1, int x2, int y2, const char *character,
                  Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000))
    {
        drawLine(x1, y1, x2, y2, character, fgColor.argb32(), bgColor.argb32());
    }
    void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000);
    void drawBox(int x, int y, int width, int height, Color fgColor,
                 Color bgColor = Color::fromArgb32(0x00000000))
    {
        drawBox(x, y, width, height, fgColor.argb32(), bgColor.argb32());
    }
    void fillRect(int x, int y, int width, int height, const char* character = " ", uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000);
    void fillRect(int x, int y, int width, int height, const char *character,
                  Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000))
    {
        fillRect(x, y, width, height, character, fgColor.argb32(), bgColor.argb32());
    }

    // Generates ANSI escape sequences using absolute cursor positioning that
    // turn this buffer's contents into nextBuffer's. The std::string overloads
    // append to a caller-owned string so the renderer can reuse its allocation.
    std::string diff(const TextGridBuffer& nextBuffer) const;
    void diff(const TextGridBuffer& nextBuffer, std::string& out,
              const AnsiEncodeOptions& options = {}) const;

    // Same with relative cursor movements from the region's top-left corner.
    std::string diffRelative(const TextGridBuffer& nextBuffer) const;
    void diffRelative(const TextGridBuffer& nextBuffer, std::string& out,
                      const AnsiEncodeOptions& options = {}) const;

    // Converts the buffer linearly to an ANSI string with newlines.
    std::string toAnsiString() const;
    void toAnsiString(std::string& out) const;

    // Column of the last cell that shows when the row is printed: a glyph
    // other than space, a non-default background or an attribute. -1 when the
    // row is blank.
    int lastContentColumn(int row) const;
    // Appends the row as ANSI text up to lastContentColumn(row), starting from
    // a style reset. No cursor movement or line break is emitted.
    void appendRowAnsi(std::string& out, int row, ColorDepth depth) const;

private:
    void diffInternal(const TextGridBuffer& nextBuffer, bool absolute, std::string& out,
                      const AnsiEncodeOptions& options) const;

    int m_width = 0;
    int m_height = 0;
    std::vector<Cell> m_cells;
};

} // namespace qamui
