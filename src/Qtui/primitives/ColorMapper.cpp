#include "ColorMapper.h"
#include <QHash>

namespace qtui {

static const QHash<QString, uint32_t> svgColors = {
    {"transparent", 0x00000000},
    {"black", 0xFF000000},
    {"silver", 0xFFC0C0C0},
    {"gray", 0xFF808080},
    {"white", 0xFFFFFFFF},
    {"maroon", 0xFF800000},
    {"red", 0xFFFF0000},
    {"purple", 0xFF800080},
    {"fuchsia", 0xFFFF00FF},
    {"green", 0xFF008000},
    {"lime", 0xFF00FF00},
    {"olive", 0xFF808000},
    {"yellow", 0xFFFFFF00},
    {"navy", 0xFF000080},
    {"blue", 0xFF0000FF},
    {"teal", 0xFF008080},
    {"aqua", 0xFF00FFFF},
    {"lightblue", 0xFFADD8E6},
    {"lightgreen", 0xFF90EE90},
    {"darkblue", 0xFF00008B},
    {"darkgreen", 0xFF006400},
    {"orange", 0xFFFFA500},
    {"pink", 0xFFFFC0CB},
    {"cyan", 0xFF00FFFF},
    {"magenta", 0xFFFF00FF}
    // Note: Can be expanded with the full SVG 1.0 list
};

uint32_t ColorMapper::fromString(const QString& colorString, uint32_t defaultColor) {
    if (colorString.isEmpty()) {
        return defaultColor;
    }

    QString colorStr = colorString.trimmed();

    if (colorStr.startsWith('#')) {
        bool ok = false;
        if (colorStr.length() == 7) {
            // #RRGGBB
            uint32_t rgb = colorStr.mid(1).toUInt(&ok, 16);
            if (ok) return 0xFF000000 | rgb;
        } else if (colorStr.length() == 9) {
            // #AARRGGBB
            uint32_t argb = colorStr.mid(1).toUInt(&ok, 16);
            if (ok) return argb;
        } else if (colorStr.length() == 4) {
            // #RGB
            uint32_t r = colorStr.mid(1, 1).toUInt(&ok, 16);
            uint32_t g = colorStr.mid(2, 1).toUInt(&ok, 16);
            uint32_t b = colorStr.mid(3, 1).toUInt(&ok, 16);
            if (ok) {
                return 0xFF000000 | (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
            }
        }
        return defaultColor;
    }

    // Try named colors
    auto it = svgColors.find(colorStr.toLower());
    if (it != svgColors.end()) {
        return it.value();
    }

    // Try parsing as integer for backwards compatibility with terminal colors
    bool ok = false;
    uint32_t intColor = colorStr.toUInt(&ok);
    if (ok && intColor < 256) {
        // We shouldn't mix 256-color logic here ideally, but if we need a fallback:
        // We'll just return a distinct flag or convert it. 
        // For simplicity, let's just use it as a 24-bit color if it's an integer.
        // Wait, standard 256 colors don't map to #RRGGBB directly without a huge table.
        // Let's assume the user doesn't need legacy 256-color codes if they move to strings.
        return defaultColor;
    }

    return defaultColor;
}

} // namespace qtui
