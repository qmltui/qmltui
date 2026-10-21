# TextGridBuffer

`TextGridBuffer` is QmlTui's off-screen 2D character-cell canvas. It stores a contiguous grid of [`Cell`](cell.md) values and provides grapheme-aware drawing helpers (`setGrapheme`, `writeText`, `drawLine`, `drawBox`, `fillRect`) together with differential ANSI sequence encoders (`diff`, `diffRelative`).

New cells and default foreground arguments use `0x00000000` (the terminal default foreground). Pass `0xFFFFFFFF` or `Color("white")` for literal RGB white. Zero-alpha backgrounds (`0x00000000` or `Color("transparent")`) preserve the existing cell background underneath.

- **Header**: `<QmlTui/Core/Primitives/TextGridBuffer.h>`
- **Target**: `qmltui::core`

## `AnsiEncodeOptions`

Controls how `diff()` and `diffRelative()` serialize cell changes into VT/ANSI escape sequences:

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `colorDepth` | `ColorDepth` | `ColorDepth::TrueColor` | Target color encoding (`Mono`, `Basic16`, `Indexed256`, or `TrueColor`). |
| `eraseToEndOfLine` | `bool` | `true` | Emit `\033[K` (EL) instead of a trailing run of default-background spaces. |
| `eraseMinimumCells` | `int` | `4` | Minimum trailing blank cells required before `\033[K` is used. |

## Public Methods

#### Grid Geometry and Cell Access

- `bool resize(int width, int height)` — resizes the buffer in columns and rows and resets all cells to default spaces. Returns `true` if dimensions changed.
- `void clear()` — resets all cells to default spaces and clears the per-frame OSC 8 hyperlink table.
- `void swap(TextGridBuffer &other) noexcept` — swaps contents with `other` in O(1).
- `int width() const` / `int height() const` — buffer dimensions in columns and rows.
- `bool contains(int x, int y) const` — returns `true` when `(x, y)` is inside the buffer bounds.
- `Cell *tryCellAt(int x, int y)` / `const Cell *tryCellAt(int x, int y) const` — returns a pointer to the cell at `(x, y)`, or `nullptr` if out of bounds.
- `Cell &cellAt(int x, int y)` / `const Cell &cellAt(int x, int y) const` — unchecked reference access (asserts in debug builds).

#### Drawing Primitives

Overloads are provided for both raw `uint32_t` ARGB values and [`qmltui::Color`](color.md):

- `int setGrapheme(int x, int y, QByteArrayView utf8, int width, ...)` / `int setGrapheme(int x, int y, QStringView cluster, int width, ...)` — writes a pre-segmented grapheme cluster of display width `width` at `(x, y)` (plus `width - 1` continuation cells), returning the column advance (`width`, `0` when appended as a zero-width combining mark to the preceding cell, or `-1` when it does not fit).
- `void writeText(int x, int y, const QString &text, ...)` — segments `text` into grapheme clusters and writes them horizontally starting at `(x, y)`.
- `void drawLine(int x1, int y1, int x2, int y2, const char *character = "*", ...)` — draws a line using Bresenham's algorithm.
- `void drawBox(int x, int y, int width, int height, ...)` — draws a single-line box (`┌ ┐ └ ┘ ─ │`).
- `void fillRect(int x, int y, int width, int height, const char *character = " ", ...)` — fills a rectangular region.

#### ANSI Serialization and Hyperlinks

- `std::string diff(const TextGridBuffer &nextBuffer) const` / `void diff(const TextGridBuffer &nextBuffer, std::string &out, const AnsiEncodeOptions &options = {}) const` — compares this buffer with `nextBuffer` and emits 1-based absolute cursor moves and SGR diffs for modified cells.
- `std::string diffRelative(const TextGridBuffer &nextBuffer) const` / `void diffRelative(const TextGridBuffer &nextBuffer, std::string &out, const AnsiEncodeOptions &options = {}) const` — emits relative cursor movement sequences for `InlineRenderer`.
- `std::string toAnsiString() const` / `void toAnsiString(std::string &out) const` — serializes the full buffer into newline-separated ANSI rows.
- `int lastContentColumn(int row) const` / `void appendRowAnsi(std::string &out, int row, ColorDepth depth) const` — row-level serialization helpers used by scrollback output.
- `uint8_t registerLink(const QString &url)` / `QString linkUrl(uint8_t linkId) const` / `void setActiveLinkId(uint8_t linkId)` / `uint8_t activeLinkId() const` — per-frame OSC 8 hyperlink table (`1..255`; `0` means no link).

## Example

```cpp
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <iostream>

int main() {
    qmltui::TextGridBuffer buffer;
    buffer.resize(40, 10);
    buffer.drawBox(0, 0, 40, 10, qmltui::Color(QStringLiteral("green")));
    buffer.writeText(2, 1, QStringLiteral("QmlTui Terminal Grid"),
                     qmltui::Color(QStringLiteral("white")),
                     qmltui::Color(QStringLiteral("transparent")),
                     qmltui::TextAttribute::Bold);
    std::cout << buffer.toAnsiString() << std::endl;
    return 0;
}
```
