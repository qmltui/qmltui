#pragma once

#include <cstdint>
#include <array>

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
    std::array<char, 4> character{'\0', '\0', '\0', '\0'};
    uint32_t fgColor = 0xFFFFFFFF; // Default foreground (White)
    uint32_t bgColor = 0x00000000; // Default background (Transparent)
    TextAttribute attributes = TextAttribute::None;

    bool operator==(const Cell& other) const {
        return character == other.character &&
               fgColor == other.fgColor &&
               bgColor == other.bgColor &&
               attributes == other.attributes;
    }
    bool operator!=(const Cell& other) const {
        return !(*this == other);
    }
};

} // namespace qtui
