#pragma once

#include <QString>
#include <cstdint>

#include "Color.h"

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

    // Nearest xterm-256 palette index (16..255: the 6x6x6 cube and the grey
    // ramp; the 16 system colours are skipped because terminals theme them).
    static uint8_t toIndexed256(uint32_t argb);

    // Nearest of the 16 standard ANSI colours (0..15, xterm default palette).
    static uint8_t toBasic16(uint32_t argb);
};

} // namespace qtui
