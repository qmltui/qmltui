# TextGridBuffer

## 1. Class Overview

`TextGridBuffer` is the core off-screen rendering canvas for Qtui. It represents a 2D grid of terminal cells (`Cell`), where each cell stores UTF-8 character bytes, visual display width, continuation flag, 32-bit ARGB foreground/background colors, and text formatting attributes (bold, italic, underline, reversed).

`TextGridBuffer` provides low-level character drawing primitives (`writeText`, `drawLine`, `drawBox`, `fillRect`) and efficient differential ANSI sequence generation (`diff`, `diffRelative`). The rendering engine uses `TextGridBuffer` to compare the previous frame with the current frame and emit minimal ANSI escape codes to the terminal output.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/core/TextGridBuffer.h`
- **Implementation**: `Terminal/src/core/TextGridBuffer.cpp`
- **Dependencies**:
  - `Cell.h` — provides `Cell` struct and `TextAttribute` enum.
  - Standard C++ Library (`<vector>`, `<string>`).
  - Qt Core (`QString`).

## 3. Class Hierarchy and Role

- **Base Class**: None (standalone value/utility class).
- **Role**: Serves as the memory buffer for 2D terminal character rendering and ANSI delta calculations.

## 9. Public Methods

#### bool resize(int width, int height)
Resizes the 2D cell grid buffer to the specified width and height in terminal character columns and rows. All cells are reset to empty spaces with default formatting. Returns `true` if the size changed, otherwise `false`.

#### void clear()
Clears the contents of the buffer, filling every cell with space characters and resetting colors and attributes to defaults.

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

#### void writeText(int x, int y, const QString& text, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000, TextAttribute attributes = TextAttribute::None)
Writes a UTF-8 encoded string horizontally starting at coordinate (x, y). An
overload accepts public `Color` foreground and background values.

#### void drawLine(int x1, int y1, int x2, int y2, const char* character = "*", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000)
Draws a straight line from (x1, y1) to (x2, y2) using Bresenham's line algorithm with the specified character string and colors.

#### void drawBox(int x, int y, int width, int height, uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000)
Draws a hollow rectangular box with single-line UTF-8 box-drawing border characters (`┌ ┐ └ ┘ ─ │`).

#### void fillRect(int x, int y, int width, int height, const char* character = " ", uint32_t fgColor = 0xFFFFFFFF, uint32_t bgColor = 0x00000000)
Fills a rectangular area starting at (x, y) with the given character and background/foreground colors.

#### std::string diff(const TextGridBuffer& nextBuffer) const
Compares this buffer with `nextBuffer` cell-by-cell and returns an ANSI escape sequence string using absolute cursor positioning commands to update only modified cells.

#### std::string diffRelative(const TextGridBuffer& nextBuffer) const
Compares this buffer with `nextBuffer` and returns relative cursor movement ANSI escape sequences.

#### std::string toAnsiString() const
Serializes the entire grid buffer into a formatted ANSI string complete with newline characters per row.

## 11. Ownership and Lifecycle

`TextGridBuffer` manages its internal `std::vector<Cell>` via standard value semantics. It is safe to copy or move `TextGridBuffer` instances.

## 12. Thread Safety

`TextGridBuffer` is **single-threaded**. Instances should be accessed from a single thread (typically the main/rendering thread).

## 14. Inter-Class Interactions

- **`Item::paint()` / `paintOverlay()`**: Receive a reference to `TextGridBuffer` while the framework paints an item tree.
- **`FrameRenderer` / `TerminalEngine`**: Maintains front and back `TextGridBuffer` instances to calculate frame diffs.

## 16. Usage Example

```cpp
#include "TextGridBuffer.h"
#include <iostream>

using namespace qtui;

int main() {
    TextGridBuffer buffer;
    buffer.resize(40, 10);

    // Draw a box and header text
    buffer.drawBox(0, 0, 40, 10, Color("green"));
    buffer.writeText(2, 1, "Qtui Terminal Grid", Color("white"),
                     Color("transparent"), TextAttribute::Bold);

    // Convert to ANSI string and print to standard output
    std::cout << buffer.toAnsiString() << std::endl;
    return 0;
}
```
