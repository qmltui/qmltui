# Subpixel Helpers (`braille`, `half_block`, `block_glyphs`)

`<QmlTui/Core/Primitives/Subpixel.h>` provides compile-time constants and zero-allocation UTF-8 glyph helpers for custom C++ items that render sub-character graphics using Unicode Braille (`2x4` dots per cell) or vertical half-block (`1x2` subpixels per cell) encodings.

- **Header**: `<QmlTui/Core/Primitives/Subpixel.h>`
- **Target**: `QmlTui::Core`

## `qmltui::braille` (`2x4` Dot Matrix)

Each terminal character cell maps to a `2` column by `4` row Braille pattern in `U+2800..U+28FF`:

| Symbol | Type / Signature | Description |
| :--- | :--- | :--- |
| `braille::DotsX` | `inline constexpr int` (`2`) | Horizontal subpixel dots per terminal cell. |
| `braille::DotsY` | `inline constexpr int` (`4`) | Vertical subpixel dots per terminal cell. |
| `braille::dotMask(int dotX, int dotY)` | `constexpr uint8_t` | Bitmask for `(dotX, dotY)` where `0 <= dotX < 2` and `0 <= dotY < 4`. Combine multiple active dots in a cell with bitwise OR (`\|`). |
| `braille::utf8(uint8_t mask)` | `QByteArrayView` | Returns the static 3-byte UTF-8 Braille sequence for `mask` (`U+2800 + mask`), valid for the lifetime of the process. |

## `qmltui::half_block` and `qmltui::block_glyphs`

Vertical half-block rendering packs two vertically stacked TrueColor pixels into a single cell by pairing `UpperHalf` (`▀`) or `LowerHalf` (`▄`) with distinct foreground and background colors:

| Symbol | Value | Description |
| :--- | :--- | :--- |
| `half_block::SubpixelsX` | `1` | Horizontal subpixels per terminal cell. |
| `half_block::SubpixelsY` | `2` | Vertical subpixels per terminal cell. |
| `block_glyphs::UpperHalf` | `"▀"` (`\xE2\x96\x80`) | Upper half block glyph. |
| `block_glyphs::LowerHalf` | `"▄"` (`\xE2\x96\x84`) | Lower half block glyph. |
| `block_glyphs::Full` | `"█"` (`\xE2\x96\x88`) | Full block glyph. |

## Example: Painting a Braille Wave into `TextGridBuffer`

```cpp
#include <QmlTui/Core/Primitives/Subpixel.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

void WaveItem::paint(qmltui::TextGridBuffer& buffer, int absX, int absY,
                     const qmltui::ClipRect& clipRect) const {
    for (int col = 0; col < width(); ++col) {
        if (!clipRect.contains(absX + col, absY))
            continue;
        uint8_t mask = qmltui::braille::dotMask(0, col % qmltui::braille::DotsY)
                     | qmltui::braille::dotMask(1, (col + 1) % qmltui::braille::DotsY);
        const QByteArrayView glyph = qmltui::braille::utf8(mask);
        buffer.setGrapheme(absX + col, absY, glyph, 1, m_color);
    }
}
```
