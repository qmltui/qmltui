# Cell & TextAttribute

## 1. Overview

`Cell` and `TextAttribute` define the primitive data structures representing a single terminal grid position in Qtui. A `Cell` stores up to 4 bytes of a UTF-8 character, 32-bit ARGB foreground and background colors, and styling attributes (bold, italic, underline, reversed).

Developers interact with `Cell` when implementing custom C++ terminal UI items.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/core/Cell.h`
- **Dependencies**: `<cstdint>`, `<array>`.
- **Module**: Part of the core Qtui library layer.

## 5. Enumerations

### TextAttribute

| Value | Integer | Description |
|-------|---------|-------------|
| `None` | `0` | Default plain text with no special styling attributes. |
| `Bold` | `1` | Renders text in bold font (`\033[1m`). |
| `Italic` | `2` | Renders text in italic font (`\033[3m`). |
| `Underline` | `4` | Renders text with an underline (`\033[4m`). |
| `Reversed` | `8` | Inverts foreground and background colors (`\033[7m`). |

Bitwise operators `|` and `&` are overloaded to allow combining multiple `TextAttribute` flags (e.g. `TextAttribute::Bold | TextAttribute::Underline`).

## 6. Public Member Variables

### Cell Struct

| Variable | Type | Description |
|----------|------|-------------|
| `character` | `std::array<char, 4>` | Fixed-size 4-byte buffer storing a single UTF-8 encoded character. |
| `fgColor` | `uint32_t` | 32-bit ARGB foreground color value (default: `0xFFFFFFFF` white). |
| `bgColor` | `uint32_t` | 32-bit ARGB background color value (default: `0x00000000` transparent). |
| `attributes` | `TextAttribute` | Bitwise combination of formatting flags applied to this cell. |

## 11. Ownership and Lifecycle

`Cell` is a lightweight plain-old-data (POD) struct designed for fast value copying and zero-heap allocation inside `TextGridBuffer`.

## 12. Thread Safety

`Cell` has no internal synchronisation. It inherits the thread-safety profile of its containing `TextGridBuffer`.

## 16. Usage Example

```cpp
#include "Cell.h"
#include <iostream>

using namespace qtui;

int main() {
    Cell cell;
    cell.character = {'X', '\0', '\0', '\0'};
    cell.fgColor = 0xFFFF0000; // Red
    cell.attributes = TextAttribute::Bold | TextAttribute::Underline;

    if (cell.attributes & TextAttribute::Bold) {
        std::cout << "Cell is bold!" << std::endl;
    }
    return 0;
}
```
