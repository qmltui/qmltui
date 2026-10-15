#pragma once

#include <QmlTui/Core/Primitives/Cell.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <vector>
#include <string>
#include <QString>
#include <QStringList>

namespace qmltui {

// How a frame diff is encoded for the terminal.
struct AnsiEncodeOptions {
    ColorDepth colorDepth = ColorDepth::TrueColor;
    bool eraseToEndOfLine = true; // "\033[K" instead of a run of trailing blanks
    int eraseMinimumCells = 4;    // only when at least this many blanks remain
};

/**
 * @brief Double-buffered 2D character grid representing the terminal matrix.
 *
 * TextGridBuffer holds an array of @ref Cell elements indexed by (column, row).
 * Visual items paint into a TextGridBuffer during frame commit. The terminal
 * engine compares the front and back buffers via @ref diff() to produce minimal
 * VT/ANSI escape sequences that update only modified cells.
 *
 * Supports full-width Unicode characters, East Asian ambiguous width, grapheme
 * clustering, and 24-bit TrueColor/ANSI 256 color mapping.
 *
 * @thread_safety Not thread-safe. Manipulated on the QmlTui engine render thread.
 * @see Cell, Color, TextAttribute
 */
class TextGridBuffer {
public:
    TextGridBuffer();

    /// @brief Resizes buffer to given columns and rows, clearing contents. Returns true if size
    /// changed.
    bool resize(int width, int height);
    /// @brief Resets all cells to blank spaces with default background and foreground.
    void clear();
    /// @brief Swaps contents with another buffer in O(1) time without copying cell vectors.
    void swap(TextGridBuffer& other) noexcept;

    /// @brief Width of the buffer in character columns.
    int width() const { return m_width; }
    /// @brief Height of the buffer in character rows.
    int height() const { return m_height; }

    /// @brief Returns true if (x, y) column/row coordinates are within bounds [0, width) x [0,
    /// height).
    bool contains(int x, int y) const;
    /// @brief Returns pointer to cell at (x, y), or nullptr if out of bounds.
    Cell* tryCellAt(int x, int y);
    /// @brief Const overload of tryCellAt(). Returns nullptr if out of bounds.
    const Cell* tryCellAt(int x, int y) const;

    /// @brief Returns reference to cell at (x, y). Undefined behavior if out of bounds.
    Cell& cellAt(int x, int y);
    /// @brief Const overload of cellAt(). Undefined behavior if out of bounds.
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
    int setGrapheme(int x, int y, QByteArrayView utf8, int width, uint32_t fgColor = 0x00000000,
                    uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    int setGrapheme(int x, int y, QByteArrayView utf8, int width, Color fgColor,
                    Color bgColor = Color::fromArgb32(0x00000000),
                    TextAttribute attributes = TextAttribute::None) {
        return setGrapheme(x, y, utf8, width, fgColor.paintedArgb32(), bgColor.paintedArgb32(),
                           attributes);
    }

    // Same as above for a UTF-16 cluster (typically a GraphemeRef::utf16 view);
    // control characters are replaced by their visible glyphs as in writeText.
    int setGrapheme(int x, int y, QStringView cluster, int width, uint32_t fgColor = 0x00000000,
                    uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    int setGrapheme(int x, int y, QStringView cluster, int width, Color fgColor,
                    Color bgColor = Color::fromArgb32(0x00000000),
                    TextAttribute attributes = TextAttribute::None) {
        return setGrapheme(x, y, cluster, width, fgColor.paintedArgb32(), bgColor.paintedArgb32(),
                           attributes);
    }

    // Drawing and text helpers. Omitted foregrounds use the terminal default;
    // zero-alpha backgrounds preserve the surface underneath.
    void writeText(int x, int y, const QString& text, uint32_t fgColor = 0x00000000,
                   uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None);
    void writeText(int x, int y, const QString& text, Color fgColor,
                   Color bgColor = Color::fromArgb32(0x00000000),
                   TextAttribute attributes = TextAttribute::None) {
        writeText(x, y, text, fgColor.paintedArgb32(), bgColor.paintedArgb32(), attributes);
    }
    void drawLine(int x1, int y1, int x2, int y2, const char* character = "*",
                  uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000);
    void drawLine(int x1, int y1, int x2, int y2, const char* character, Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000)) {
        drawLine(x1, y1, x2, y2, character, fgColor.paintedArgb32(), bgColor.paintedArgb32());
    }
    void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0x00000000,
                 uint32_t bgColor = 0x00000000);
    void drawBox(int x, int y, int width, int height, Color fgColor,
                 Color bgColor = Color::fromArgb32(0x00000000)) {
        drawBox(x, y, width, height, fgColor.paintedArgb32(), bgColor.paintedArgb32());
    }
    void fillRect(int x, int y, int width, int height, const char* character = " ",
                  uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000);
    void fillRect(int x, int y, int width, int height, const char* character, Color fgColor,
                  Color bgColor = Color::fromArgb32(0x00000000)) {
        fillRect(x, y, width, height, character, fgColor.paintedArgb32(), bgColor.paintedArgb32());
    }

    /**
     * @brief Generates minimal VT/ANSI escape sequences to update a terminal
     * from this buffer's state to @p nextBuffer's state.
     *
     * Uses absolute cursor positioning. Appends to caller-owned string in the
     * second overload to avoid heap allocations across frames.
     */
    std::string diff(const TextGridBuffer& nextBuffer) const;
    void diff(const TextGridBuffer& nextBuffer, std::string& out,
              const AnsiEncodeOptions& options = {}) const;

    /**
     * @brief Computes relative ANSI cursor diff from top-left corner.
     *
     * Used for inline stream rendering where the cursor origin is relative.
     */
    std::string diffRelative(const TextGridBuffer& nextBuffer) const;
    void diffRelative(const TextGridBuffer& nextBuffer, std::string& out,
                      const AnsiEncodeOptions& options = {}) const;

    /**
     * @brief Formats the entire buffer as a complete ANSI text string with newlines.
     *
     * Suitable for full-screen flushes, headless snapshots, or terminal dump logging.
     */
    std::string toAnsiString() const;
    void toAnsiString(std::string& out) const;

    // Column of the last cell that shows when the row is printed: a glyph
    // other than space, a non-default background or an attribute. -1 when the
    // row is blank.
    int lastContentColumn(int row) const;
    // Appends the row as ANSI text up to lastContentColumn(row), starting from
    // a style reset. No cursor movement or line break is emitted.
    void appendRowAnsi(std::string& out, int row, ColorDepth depth) const;

    uint8_t registerLink(const QString& url);
    QString linkUrl(uint8_t linkId) const;
    void setActiveLinkId(uint8_t linkId) { m_activeLinkId = linkId; }
    uint8_t activeLinkId() const { return m_activeLinkId; }

private:
    void diffInternal(const TextGridBuffer& nextBuffer, bool absolute, std::string& out,
                      const AnsiEncodeOptions& options) const;

    int m_width = 0;
    int m_height = 0;
    std::vector<Cell> m_cells;
    QStringList m_links;
    uint8_t m_activeLinkId = 0;
};

} // namespace qmltui
