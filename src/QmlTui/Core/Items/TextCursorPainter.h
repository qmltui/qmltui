#pragma once

// Internal helper shared by TextInput and TextArea. Not installed: it must
// only be included from .cpp files.

#include <QmlTui/Core/Primitives/Cell.h>
#include <QmlTui/Core/Primitives/Color.h>

#include <cstdint>
#include <optional>

namespace qmltui::detail {

enum class TextCursorShape : uint8_t {
    Block,
    Underline,
    Beam,
    HollowBlock,
};

// A cursorColor that is unset, "default" (alpha 0, the terminal's own colour)
// or "transparent" has no ink of its own: the cursor then uses reverse video,
// which is readable on every terminal theme.
inline std::optional<uint32_t> resolveCursorColor(Color color)
{
    if (!color.isValid() || color.isTransparent() || (color.argb32() >> 24) == 0)
        return std::nullopt;
    return color.argb32();
}

inline bool isBlankCursorCell(const Cell &cell)
{
    return cell.character().isEmpty() || cell.character() == QByteArrayView(" ");
}

inline bool hasAttribute(const Cell &cell, TextAttribute attribute)
{
    return (cell.attributes & attribute) != TextAttribute::None;
}

// A block cursor is reverse video. With a cursor colour, that colour becomes
// the ink before reversal, so it fills the cell and the glyph is drawn in the
// cell's own background (the terminal background when unset). Without one,
// Reversed is toggled so the cursor stays visible inside a selection.
inline void paintBlockCursor(Cell &cell, std::optional<uint32_t> color)
{
    if (cell.character().isEmpty())
        cell.setCharacter(" ");
    if (color) {
        cell.bgColor = *color;
        cell.fgColor = 0xFF000000u;
    } else {
        cell.attributes = static_cast<TextAttribute>(
            static_cast<uint8_t>(cell.attributes)
            ^ static_cast<uint8_t>(TextAttribute::Reversed));
    }
}

// Paints a glyph-shaped cursor into a blank cell.
inline void paintGlyphCursor(Cell &cell, QByteArrayView glyph, std::optional<uint32_t> color)
{
    cell.setCharacter(glyph);
    if (color)
        cell.fgColor = *color;
}

// Marks an occupied cell by underlining it. A cell that is already underlined
// (for example by a highlight range) would show no change, so it falls back
// to a block cursor.
inline void paintUnderlineOverGlyph(Cell &cell, std::optional<uint32_t> color)
{
    if (hasAttribute(cell, TextAttribute::Underline)) {
        paintBlockCursor(cell, color);
        return;
    }
    if (color)
        cell.fgColor = *color;
    cell.attributes = cell.attributes | TextAttribute::Underline;
}

// A terminal cell holds one glyph, so a bar or an outline can only be drawn in
// a blank cell. Over a character, Beam renders as a block and Underline and
// HollowBlock underline the character.
inline void paintTextCursor(Cell &cell, TextCursorShape shape, std::optional<uint32_t> color)
{
    const bool blank = isBlankCursorCell(cell);
    switch (shape) {
    case TextCursorShape::Block:
        paintBlockCursor(cell, color);
        break;
    case TextCursorShape::Underline:
        if (blank)
            paintGlyphCursor(cell, "_", color);
        else
            paintUnderlineOverGlyph(cell, color);
        break;
    case TextCursorShape::Beam:
        if (blank)
            paintGlyphCursor(cell, "\xe2\x94\x82", color); // U+2502 │
        else
            paintBlockCursor(cell, color);
        break;
    case TextCursorShape::HollowBlock:
        if (blank)
            paintGlyphCursor(cell, "\xe2\x96\xaf", color); // U+25AF ▯
        else
            paintUnderlineOverGlyph(cell, color);
        break;
    }
}

} // namespace qmltui::detail
