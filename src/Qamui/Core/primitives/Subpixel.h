#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>
#include <QByteArrayView>
#include <cassert>
#include <cstdint>

namespace qamui {

namespace braille {
inline constexpr int DotsX = 2;
inline constexpr int DotsY = 4;

// Coordinates must satisfy 0 <= dotX < DotsX and 0 <= dotY < DotsY.
constexpr std::uint8_t dotMask(int dotX, int dotY) noexcept {
    assert(dotX >= 0 && dotX < DotsX && dotY >= 0 && dotY < DotsY);
    constexpr std::uint8_t masks[2][4] = {
        { 0x01, 0x02, 0x04, 0x40 },
        { 0x08, 0x10, 0x20, 0x80 }
    };
    return masks[dotX][dotY];
}

// Returns exactly three UTF-8 bytes.
// The returned view remains valid for the process lifetime.
QAMUI_CORE_EXPORT QByteArrayView utf8(std::uint8_t mask) noexcept;
} // namespace braille

namespace half_block {
inline constexpr int SubpixelsX = 1;
inline constexpr int SubpixelsY = 2;
} // namespace half_block

namespace block_glyphs {
inline constexpr char UpperHalf[] = "\xE2\x96\x80";
inline constexpr char LowerHalf[] = "\xE2\x96\x84";
inline constexpr char Full[]      = "\xE2\x96\x88";
} // namespace block_glyphs

} // namespace qamui
