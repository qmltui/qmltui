#pragma once

#include <QString>
#include <cstdint>

namespace qtui {

class ColorMapper {
public:
    // Parses a color string (e.g., "#FFFFFF", "blue") and returns a 32-bit ARGB value.
    // If the color cannot be parsed, returns the defaultColor.
    static uint32_t fromString(const QString& colorString, uint32_t defaultColor = 0xFFFFFFFF);
};

} // namespace qtui
