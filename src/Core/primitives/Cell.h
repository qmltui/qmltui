#pragma once

#include <cstdint>
#include <QByteArray>

namespace qtui {

enum class TextAttribute : uint8_t {
    None      = 0,
    Bold      = 1 << 0,
    Italic    = 1 << 1,
    Underline = 1 << 2,
    Reversed  = 1 << 3
};

inline TextAttribute operator|(TextAttribute a, TextAttribute b) {
    return static_cast<TextAttribute>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TextAttribute operator&(TextAttribute a, TextAttribute b) {
    return static_cast<TextAttribute>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct Cell {
    QByteArray character{" "};
    uint8_t displayWidth = 1;
    bool continuation = false;
    uint32_t fgColor = 0xFFFFFFFF; // Default foreground (White)
    uint32_t bgColor = 0x00000000; // Default background (Transparent)
    TextAttribute attributes = TextAttribute::None;

    bool operator==(const Cell& other) const {
        return character == other.character &&
               displayWidth == other.displayWidth &&
               continuation == other.continuation &&
               fgColor == other.fgColor &&
               bgColor == other.bgColor &&
               attributes == other.attributes;
    }
    bool operator!=(const Cell& other) const {
        return !(*this == other);
    }
};

} // namespace qtui
