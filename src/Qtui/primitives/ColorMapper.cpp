#include "ColorMapper.h"
#include "Color.h"

namespace qtui {

uint32_t ColorMapper::fromString(const QString& colorString, uint32_t defaultColor) {
    const Color color = Color::fromString(colorString);
    if (color.isValid())
        return color.argb32();

    return defaultColor;
}

} // namespace qtui
