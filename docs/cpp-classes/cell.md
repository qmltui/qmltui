# Cell & TextAttribute

`Cell` and `TextAttribute` represent a single character-grid position in `TextGridBuffer`. A `Cell` stores the UTF-8 bytes of one grapheme cluster, its display column width, a continuation flag for wide characters, 32-bit ARGB foreground and background colors, and SGR formatting attributes.

- **Header**: `<QmlTui/Core/Primitives/Cell.h>`
- **Target**: `QmlTui::Core`

## Storage Model

`Cell` is 24 bytes and trivially copyable. Grapheme clusters up to `Cell::InlineGlyphCapacity` (11 bytes) are stored inline, covering single code points, emoji with skin tones or variation selectors, flags, and keycaps. Longer clusters (such as multi-person ZWJ sequences) are interned in a bounded process-wide table (`Cell::MaxInternedGlyphs = 16384` unique entries, each capped at `Cell::MaxGraphemeBytes = 64` bytes) and referenced by index, keeping memory growth strictly bounded in long-lived services. Because `Cell` has no per-instance heap allocation, cells copy and compare by value and `TextGridBuffer::clear()` is a contiguous fill.

## `TextAttribute`

| Value | Integer | Description |
| :--- | :--- | :--- |
| `None` | `0` | Default plain text. |
| `Bold` | `1` | Bold intensity (SGR 1). |
| `Italic` | `2` | Italic style (SGR 3). |
| `Underline` | `4` | Underlined text (SGR 4). |
| `Reversed` | `8` | Swaps foreground and background colors (SGR 7). |
| `Blink` | `16` | Blinking text (SGR 5). |
| `Conceal` | `32` | Concealed text (SGR 8). |
| `Strikethrough` | `64` | Crossed-out text (SGR 9). |
| `Dim` | `128` | Faint intensity (SGR 2). |

Bitwise `|` and `&` operators combine and test `TextAttribute` flags (for example, `TextAttribute::Bold | TextAttribute::Underline`).

## Public Member Variables (`Cell`)

| Member | Type | Description |
| :--- | :--- | :--- |
| `InlineGlyphCapacity` | `static constexpr int` | Maximum UTF-8 byte length (`11`) stored inline before interning. |
| `MaxGraphemeBytes` | `static constexpr int` | Maximum UTF-8 byte length (`64`) stored per grapheme cluster. |
| `MaxInternedGlyphs` | `static constexpr size_t` | Hard upper bound (`16384`) on unique multi-byte clusters retained in the process-wide intern table. |
| `fgColor` | `uint32_t` | 32-bit ARGB foreground color (`0x00000000` represents the terminal default foreground). |
| `bgColor` | `uint32_t` | 32-bit ARGB background color (`0x00000000` represents transparent / terminal default background). |
| `displayWidth` | `uint8_t` | Column width occupied by the grapheme cluster (default `1`). |
| `continuation` | `bool` | `true` on trailing cells of a multi-column character (default `false`). |
| `attributes` | `TextAttribute` | Bitwise combination of formatting flags (default `TextAttribute::None`). |
| `linkId` | `uint8_t` | Per-frame OSC 8 hyperlink table index (`0` means no link; default `0`). |

## Public Methods

#### `QByteArrayView character() const noexcept`
Returns the UTF-8 bytes of the grapheme cluster (default `" "`; empty on continuation cells). The view remains valid until the cell is mutated or its containing buffer is resized, cleared, swapped, or destroyed.

#### `void setCharacter(QByteArrayView utf8)`
Replaces the cell's grapheme cluster bytes.

#### `void appendCharacter(QByteArrayView utf8)`
Appends UTF-8 bytes to the existing cluster (used for combining marks and variation selectors).

#### `void clearCharacter() noexcept`
Empties the cluster bytes (used when marking continuation cells).

#### `bool hasCharacter() const noexcept`
Returns `true` if the cluster is non-empty.

#### `void setForeground(Color)` / `void setBackground(Color)`
#### `Color foreground() const` / `Color background() const`
Read and write `fgColor` and `bgColor` using `qmltui::Color`.

#### `bool operator==(const Cell &other) const noexcept` / `bool operator!=(const Cell &other) const noexcept`
Compares all fields, including grapheme cluster bytes and `linkId`.

## Example

```cpp
#include <QmlTui/Core/Primitives/Cell.h>

qmltui::Cell cell;
cell.setCharacter("X");
cell.setForeground(qmltui::Color(QStringLiteral("red")));
cell.attributes = qmltui::TextAttribute::Bold | qmltui::TextAttribute::Underline;
```
