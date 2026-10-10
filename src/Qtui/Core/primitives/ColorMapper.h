#pragma once

#include <QString>
#include <cstdint>

#include <Qtui/Core/primitives/Color.h>

namespace qtui {

class ColorMapper {
public:
    // Parses a color string (e.g., "#FFFFFF", "blue") and returns a 32-bit ARGB value.
    // If the color cannot be parsed, returns the defaultColor.
    static uint32_t fromString(const QString& colorString, uint32_t defaultColor = 0xFFFFFFFF);
    static uint32_t fromString(Color color, uint32_t defaultColor = 0xFFFFFFFF)
    {
        return color.isValid() ? color.argb32() : defaultColor;
    }

    // ANSI-indexed values (Color::AnsiTag in the alpha byte) carry a palette
    // index, not RGB. These helpers let RGB consumers resolve them.
    static bool isAnsiIndex(uint32_t argb) { return (argb >> 24) == 0x01U; }
    // Color::TransparentTag: paint nothing, as opposed to alpha 0, which is
    // the terminal's own default colour and is painted.
    static bool isTransparent(uint32_t argb) { return (argb >> 24) == 0x04U; }
    // Color::paintedArgb32() for a raw value: transparency has no painted
    // representation and reads as the terminal default.
    static uint32_t toPaintedColor(uint32_t argb)
    {
        return isTransparent(argb) ? 0x00000000U : argb;
    }
    static uint8_t ansiIndex(uint32_t argb) { return static_cast<uint8_t>(argb & 0x0FU); }
    // Literal RGB (xterm default palette for indices); identity otherwise.
    static uint32_t toArgb(uint32_t argb) { return Color::fromArgb32(argb).toRgb().argb32(); }

    // Nearest xterm-256 palette index (16..255: the 6x6x6 cube and the grey
    // ramp; the 16 system colours are skipped because terminals theme them).
    static uint8_t toIndexed256(uint32_t argb);

    // Nearest of the 16 standard ANSI colours (0..15, xterm default palette).
    static uint8_t toBasic16(uint32_t argb);
};

} // namespace qtui
