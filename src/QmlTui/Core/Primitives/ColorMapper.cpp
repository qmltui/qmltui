#include <QmlTui/Core/Primitives/ColorMapper.h>
#include <QmlTui/Core/Primitives/Color.h>

namespace qmltui {

uint32_t ColorMapper::fromString(const QString& colorString, uint32_t defaultColor) {
    const Color color = Color::fromString(colorString);
    if (color.isValid())
        return color.argb32();

    return defaultColor;
}

} // namespace qmltui

namespace qmltui {

namespace {

struct Rgb {
    int r;
    int g;
    int b;
};

constexpr int cubeLevels[6] = {0, 95, 135, 175, 215, 255};

// xterm's default 16-colour palette.
constexpr Rgb basic16Palette[16] = {
    {0, 0, 0},       {128, 0, 0},   {0, 128, 0},   {128, 128, 0},
    {0, 0, 128},     {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
    {128, 128, 128}, {255, 0, 0},   {0, 255, 0},   {255, 255, 0},
    {0, 0, 255},     {255, 0, 255}, {0, 255, 255}, {255, 255, 255},
};

inline int squaredDistance(const Rgb &a, const Rgb &b)
{
    const int dr = a.r - b.r;
    const int dg = a.g - b.g;
    const int db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

inline int nearestCubeLevel(int value)
{
    int best = 0;
    int bestDistance = 1 << 30;
    for (int index = 0; index < 6; ++index) {
        const int distance = (cubeLevels[index] - value) * (cubeLevels[index] - value);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return best;
}

inline Rgb rgbOf(uint32_t argb)
{
    return {static_cast<int>((argb >> 16) & 0xFF), static_cast<int>((argb >> 8) & 0xFF),
            static_cast<int>(argb & 0xFF)};
}

} // namespace

uint8_t ColorMapper::toIndexed256(uint32_t argb)
{
    if (isAnsiIndex(argb))
        return ansiIndex(argb);   // indices 0-15 are the themed system colors
    const Rgb color = rgbOf(argb);

    const int ri = nearestCubeLevel(color.r);
    const int gi = nearestCubeLevel(color.g);
    const int bi = nearestCubeLevel(color.b);
    const Rgb cube{cubeLevels[ri], cubeLevels[gi], cubeLevels[bi]};
    const int cubeIndex = 16 + 36 * ri + 6 * gi + bi;
    const int cubeDistance = squaredDistance(color, cube);

    const int average = (color.r + color.g + color.b) / 3;
    int grayStep = (average - 8 + 5) / 10;
    if (grayStep < 0) grayStep = 0;
    if (grayStep > 23) grayStep = 23;
    const int grayLevel = 8 + 10 * grayStep;
    const int grayDistance = squaredDistance(color, Rgb{grayLevel, grayLevel, grayLevel});

    return static_cast<uint8_t>(grayDistance < cubeDistance ? 232 + grayStep : cubeIndex);
}

uint8_t ColorMapper::toBasic16(uint32_t argb)
{
    if (isAnsiIndex(argb))
        return ansiIndex(argb);
    const Rgb color = rgbOf(argb);
    int best = 0;
    int bestDistance = 1 << 30;
    for (int index = 0; index < 16; ++index) {
        const int distance = squaredDistance(color, basic16Palette[index]);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return static_cast<uint8_t>(best);
}

} // namespace qmltui
