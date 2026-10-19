# TextGridBuffer

## Class Overview

`TextGridBuffer` is the core off-screen rendering canvas for QmlTui. It represents a 2D grid of terminal cells (`Cell`), where each `Cell` stores the UTF-8 bytes of one grapheme cluster (inline, without heap allocation), visual display width, continuation flag, 32-bit ARGB foreground/background colors, and text formatting attributes (bold, italic, underline, reversed).

`TextGridBuffer` provides low-level character drawing primitives (`writeText`, `drawLine`, `drawBox`, `fillRect`) and efficient differential ANSI sequence generation (`diff`, `diffRelative`). The rendering engine uses `TextGridBuffer` to compare the previous frame with the current frame and emit minimal ANSI escape codes to the terminal output.

New cells and omitted drawing-helper foregrounds use `0x00000000`, the terminal
default foreground. Pass `0xFFFFFFFF` explicitly to request literal RGB white.
Zero-alpha drawing backgrounds preserve the surface underneath.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Primitives/TextGridBuffer.h`
- **Implementation**: `src/QmlTui/Core/Primitives/TextGridBuffer.cpp`
- **Dependencies**:
  - `Cell.h` — provides `Cell` struct and `TextAttribute` enum.
  - Standard C++ Library (`<vector>`, `<string>`).
  - Qt Core (`QString`).

## Class Hierarchy and Role

- **Base Class**: None (standalone value/utility class).
- **Role**: Serves as the memory buffer for 2D terminal character rendering and ANSI delta calculations.

## Public Types

### AnsiEncodeOptions Struct

Controls how `diff()` and `diffRelative()` encode cells into VT/ANSI escape sequences:

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `colorDepth` | `ColorDepth` | `ColorDepth::TrueColor` | Target terminal color resolution (`Mono`, `Basic16`, `Indexed256`, or `TrueColor`). |
| `eraseToEndOfLine` | `bool` | `true` | Emit `\033[K` (EL) instead of a trailing run of default-background spaces. |
| `eraseMinimumCells` | `int` | `4` | Minimum number of trailing blank cells required before `\033[K` is used. |

## Public Methods

#### bool resize(int width, int height)
Resizes the 2D cell grid buffer to the specified width and height in terminal character columns and rows. All cells are reset to empty spaces with default formatting. Returns `true` if the size changed, otherwise `false`.

#### void clear()
Clears the contents of the buffer, filling every cell with space characters, resetting colors and attributes to defaults, and clearing the per-frame OSC 8 link table.

#### void swap(TextGridBuffer& other) noexcept
Efficiently swaps the internal state of this buffer with another `TextGridBuffer` instance.

#### int width() const
Returns the current width of the grid buffer in terminal character columns.

#### int height() const
Returns the current height of the grid buffer in terminal character rows.

#### bool contains(int x, int y) const
Returns whether the coordinates identify a cell in this buffer.

#### Cell *tryCellAt(int x, int y)
Returns the addressed cell, or `nullptr` for out-of-bounds coordinates. A const
overload is also provided.

#### Cell& cellAt(int x, int y)
Returns a mutable reference to the cell at grid coordinates (x, y). Coordinates
must be valid; debug builds assert this precondition.

#### const Cell& cellAt(int x, int y) const
Returns a read-only reference to the cell at grid coordinates (x, y).

#### int setGrapheme(int x, int y, QByteArrayView utf8, int width, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None)
#### int setGrapheme(int x, int y, QStringView cluster, int width, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None)
Writes one already-segmented UTF-8 or UTF-16 grapheme cluster of display width `width` at `(x, y)` (the base cell plus `width - 1` continuation cells), returning the number of columns to advance (`width`, `0` when a zero-width cluster is appended to the preceding base cell, or `-1` when it does not fit before the right edge). Overloads accepting `Color` foreground and background values are also provided.

#### void writeText(int x, int y, const QString& text, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None)
Writes a UTF-8 encoded string horizontally starting at coordinate (x, y). An
overload accepts public `Color` foreground and background values.

#### void drawLine(int x1, int y1, int x2, int y2, const char* character = "*", uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000)
Draws a straight line from (x1, y1) to (x2, y2) using Bresenham's line algorithm with the specified character string and colors. An overload accepts `Color` values.

#### void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000)
Draws a hollow rectangular box with single-line UTF-8 box-drawing border characters (`┌ ┐ └ ┘ ─ │`). An overload accepts `Color` values.

#### void fillRect(int x, int y, int width, int height, const char* character = " ", uint32_t fgColor = 0x00000000, uint32_t bgColor = 0x00000000)
Fills a rectangular area starting at (x, y) with the given character and background/foreground colors. An overload accepts `Color` values.

#### std::string diff(const TextGridBuffer& nextBuffer) const
#### void diff(const TextGridBuffer& nextBuffer, std::string& out, const AnsiEncodeOptions& options = {}) const
Compares this buffer with `nextBuffer` cell-by-cell and produces an ANSI escape sequence string using 1-based absolute cursor positioning commands to update only modified cells. The second overload appends into `out` using `options`.

#### std::string diffRelative(const TextGridBuffer& nextBuffer) const
#### void diffRelative(const TextGridBuffer& nextBuffer, std::string& out, const AnsiEncodeOptions& options = {}) const
Compares this buffer with `nextBuffer` and produces relative cursor movement ANSI escape sequences for inline stream rendering.

#### std::string toAnsiString() const / void toAnsiString(std::string& out) const
Serializes the entire grid buffer into a formatted ANSI string complete with newline characters per row.

#### int lastContentColumn(int row) const
Returns the column index of the last cell in `row` that has visible content (a non-space glyph, non-default background, or attribute), or `-1` when the row is blank.

#### void appendRowAnsi(std::string& out, int row, ColorDepth depth) const
Appends `row` as ANSI text up to `lastContentColumn(row)` starting from a style reset, without emitting cursor movement or line breaks.

#### uint8_t registerLink(const QString& url)
#### QString linkUrl(uint8_t linkId) const
#### void setActiveLinkId(uint8_t linkId) / uint8_t activeLinkId() const
Manages the per-frame OSC 8 hyperlink table (`1..255`; `0` means no link). Cells written while `activeLinkId()` is non-zero inherit that hyperlink ID and are wrapped in `\033]8;;<url>\033\\` sequences when encoded.

## Ownership and Lifecycle

`TextGridBuffer` manages its internal `std::vector<Cell>` via standard value semantics. It is safe to copy or move `TextGridBuffer` instances.

## Thread Safety

`TextGridBuffer` is **single-threaded**. Instances should be accessed from a single thread (typically the main/rendering thread).

## Inter-Class Interactions

- **`Item::paint()` / `paintOverlay()`**: Receive a reference to `TextGridBuffer` while the framework paints an item tree.
- **`AltScreenRenderer` / `TerminalEngine`**: Maintains front and back `TextGridBuffer` instances to calculate frame diffs.

## Usage Example

```cpp
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <iostream>

using namespace qmltui;

int main() {
    TextGridBuffer buffer;
    buffer.resize(40, 10);

    // Draw a box and header text
    buffer.drawBox(0, 0, 40, 10, Color("green"));
    buffer.writeText(2, 1, "QmlTui Terminal Grid", Color("white"),
                     Color("transparent"), TextAttribute::Bold);

    // Convert to ANSI string and print to standard output
    std::cout << buffer.toAnsiString() << std::endl;
    return 0;
}
```
