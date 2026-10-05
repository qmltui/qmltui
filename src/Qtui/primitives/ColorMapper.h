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
};

} // namespace qtui
