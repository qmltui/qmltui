#include <Qamui/Core/primitives/Subpixel.h>
#include <array>

namespace qamui::braille {

namespace {
constexpr auto makeBrailleTable() {
    std::array<std::array<char, 4>, 256> table{};
    for (int i = 0; i < 256; ++i) {
        table[i][0] = static_cast<char>(0xE2);
        table[i][1] = static_cast<char>(0xA0 | (i >> 6));
        table[i][2] = static_cast<char>(0x80 | (i & 0x3F));
        table[i][3] = '\0';
    }
    return table;
}
constexpr auto BraillePatterns = makeBrailleTable();
} // namespace

QByteArrayView utf8(std::uint8_t mask) noexcept {
    return QByteArrayView(BraillePatterns[mask].data(), 3);
}

} // namespace qamui::braille
