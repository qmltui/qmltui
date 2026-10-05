# Cell & TextAttribute

## 1. Class Overview

`Cell` and `TextAttribute` define the primitive data structures representing a single terminal grid position in Qtui. A `Cell` stores the UTF-8 bytes of one grapheme cluster, its visual display width, a continuation flag (for multi-column characters), 32-bit ARGB foreground and background colors, and styling attributes (bold, italic, underline, reversed, blink, conceal, and strikethrough).

Developers interact with `Cell` when implementing custom C++ terminal UI items.

## 2. Project Structure and Dependencies

- **Header**: `src/Qtui/primitives/Cell.h` (installed as `qtui/primitives/Cell.h`)
- **Dependencies**: `<cstdint>`, `<cstring>`, `QByteArrayView`, `Color.h`.
- **Module**: Part of the core Qtui library layer.

## 3. Storage Model

`Cell` is 24 bytes and trivially copyable. The grapheme cluster is stored inline when it fits in `Cell::InlineGlyphCapacity` (12) bytes, which covers every single code point, emoji with skin tone or variation selector, flags and keycaps. Longer clusters (for example ZWJ emoji sequences) are interned once in a process-wide, append-only table and referenced by index. Cells therefore copy and compare by value across buffers, `TextGridBuffer::clear()` is a plain fill, and painting never allocates for ordinary text.

## 5. Enumerations

### TextAttribute

| Value | Integer | Description |
|-------|---------|-------------|
| `None` | `0` | Default plain text with no special styling attributes. |
| `Bold` | `1` | Renders text in bold font. |
| `Italic` | `2` | Renders text in italic font. |
| `Underline` | `4` | Renders text with an underline. |
| `Reversed` | `8` | Inverts foreground and background colors. |
| `Blink` | `16` | Requests blinking text. |
| `Conceal` | `32` | Conceals the cell contents. |
| `Strikethrough` | `64` | Draws a line through the text. |

Bitwise operators `|` and `&` are overloaded to allow combining multiple `TextAttribute` flags (e.g. `TextAttribute::Bold | TextAttribute::Underline`).

## 6. Public Member Variables

### Cell Struct

| Variable | Type | Description |
|----------|------|-------------|
| `displayWidth` | `uint8_t` | Visual column width occupied by the cluster (default: `1`). |
| `continuation` | `bool` | True if this cell is a continuation of a multi-column character (default: `false`). |
| `fgColor` | `uint32_t` | 32-bit ARGB foreground color value (default: `0xFFFFFFFF` white). |
| `bgColor` | `uint32_t` | 32-bit ARGB background color value (default: `0x00000000` transparent). |
| `attributes` | `TextAttribute` | Bitwise combination of formatting flags applied to this cell (default: `TextAttribute::None`). |

## 9. Public Methods

#### QByteArrayView character() const
Returns the UTF-8 bytes of the cluster (default `" "`; empty for continuation cells). The view is valid until the cell is modified or its buffer is resized, cleared, swapped or destroyed. Convert with `toByteArray()` or `QString::fromUtf8()` to keep the bytes.

#### void setCharacter(QByteArrayView utf8)
Replaces the cluster. Accepts string literals, `QByteArray`, and views, including a view of the cell's own bytes.

#### void appendCharacter(QByteArrayView utf8)
Appends bytes to the cluster, used for combining marks and variation selectors that belong to the preceding base character.

#### void clearCharacter()
Empties the cluster (continuation cells).

#### bool hasCharacter() const
True unless the cluster is empty.

#### void setForeground(Color) / void setBackground(Color)
#### Color foreground() const / Color background() const
Colour accessors converting between `Color` and the ARGB fields.

#### bool operator==(const Cell& other) const
Compares all fields, including the cluster bytes.

## 11. Ownership and Lifecycle

`Cell` is a trivially copyable value type with no heap allocation of its own; `TextGridBuffer` stores cells in a contiguous `std::vector<Cell>`. Interned long clusters live in a process-wide table that is never freed.

## 12. Thread Safety

`Cell` has no internal synchronisation and inherits the thread-safety profile of its containing `TextGridBuffer`. The interning table used for long clusters is mutex-protected.

## 16. Usage Example

```cpp
#include "Cell.h"
#include <iostream>

using namespace qtui;

int main() {
    Cell cell;
    cell.setCharacter("X");
    cell.fgColor = 0xFFFF0000; // Red
    cell.attributes = TextAttribute::Bold | TextAttribute::Underline;

    if ((cell.attributes & TextAttribute::Bold) != TextAttribute::None) {
        std::cout << "Cell is bold: " << cell.character().data() << std::endl;
    }
    return 0;
}
```
