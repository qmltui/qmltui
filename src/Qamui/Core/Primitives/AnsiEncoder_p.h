#pragma once

// Internal helpers shared by TextGridBuffer's diff/serialisation and the
// InlineRenderer: allocation-free integer and SGR emission into a caller-owned
// std::string, with colour quantisation for the terminal's colour depth.

#include <Qamui/Core/Primitives/Cell.h>
#include <Qamui/Core/Primitives/ColorMapper.h>
#include <Qamui/Core/Primitives/TerminalCapabilities.h>

#include <charconv>
#include <cstdint>
#include <string>

namespace qamui::ansi {

inline void appendInt(std::string &out, int value)
{
    char digits[12];
    const auto result = std::to_chars(digits, digits + sizeof digits, value);
    out.append(digits, static_cast<size_t>(result.ptr - digits));
}

// Alpha 0 means "terminal default". Color::TransparentTag reaches the encoder
// only on a cell some painter wrote anyway, and the terminal has no way to
// render "nothing", so it normalizes to the default too. Otherwise force alpha
// FF so equal RGB values compare equal regardless of the source alpha.
inline uint32_t normalizedColor(uint32_t color)
{
    if (ColorMapper::isAnsiIndex(color))
        return Color::AnsiTag | ColorMapper::ansiIndex(color);   // keep the index, never RGB
    if (ColorMapper::isTransparent(color))
        return 0u;                                               // SGR 39 / 49
    return (color >> 24) != 0 ? (0xFF000000u | (color & 0x00FFFFFFu)) : 0u;
}

// Maps a normalized colour to the value the terminal will actually receive at
// the given depth, so equal-after-quantisation colours do not emit twice.
// 0 always means default. Indexed depths tag the index so it cannot collide.
inline uint32_t mappedColor(uint32_t normalized, ColorDepth depth)
{
    if (normalized == 0)
        return 0;
    if (ColorMapper::isAnsiIndex(normalized)) {
        // An ANSI index is emitted as such at every depth except Mono, so the
        // terminal theme applies; on 256-color terminals indices 0-15 are the
        // same themed entries.
        return depth == ColorDepth::Mono ? 0 : (0x02000000u | ColorMapper::ansiIndex(normalized));
    }
    switch (depth) {
    case ColorDepth::Mono:
        return 0;
    case ColorDepth::Basic16:
        return 0x02000000u | ColorMapper::toBasic16(normalized);
    case ColorDepth::Indexed256:
        return 0x01000000u | ColorMapper::toIndexed256(normalized);
    case ColorDepth::TrueColor:
        break;
    }
    return normalized;
}

// Appends the SGR parameter(s) for a mapped colour (no ESC, no 'm').
inline void appendColorParameter(std::string &out, uint32_t mapped, ColorDepth depth,
                                 bool background)
{
    if (mapped == 0) {
        out += background ? "49" : "39";
        return;
    }
    if ((mapped >> 24) == 0x02U) {   // basic-16 index, from quantisation or an ANSI-named color
        const int index = static_cast<int>(mapped & 0xFF);
        const int base = index < 8 ? (background ? 40 : 30) : (background ? 100 : 90);
        appendInt(out, base + (index & 7));
        return;
    }
    switch (depth) {
    case ColorDepth::Mono:
        out += background ? "49" : "39";
        return;
    case ColorDepth::Basic16: {
        const int index = static_cast<int>(mapped & 0xFF);
        const int base = index < 8 ? (background ? 40 : 30) : (background ? 100 : 90);
        appendInt(out, base + (index & 7));
        return;
    }
    case ColorDepth::Indexed256:
        out += background ? "48;5;" : "38;5;";
        appendInt(out, static_cast<int>(mapped & 0xFF));
        return;
    case ColorDepth::TrueColor:
        break;
    }
    out += background ? "48;2;" : "38;2;";
    appendInt(out, static_cast<int>((mapped >> 16) & 0xFF));
    out += ';';
    appendInt(out, static_cast<int>((mapped >> 8) & 0xFF));
    out += ';';
    appendInt(out, static_cast<int>(mapped & 0xFF));
}

// Attribute SGR codes in the order the encoder has always emitted them.
struct AttributeCode {
    TextAttribute attribute;
    const char *on;
    const char *off;
};
inline constexpr AttributeCode attributeCodes[] = {
    {TextAttribute::Bold, "1", "22"},
    {TextAttribute::Dim, "2", "22"},
    {TextAttribute::Italic, "3", "23"},
    {TextAttribute::Underline, "4", "24"},
    {TextAttribute::Reversed, "7", "27"},
    {TextAttribute::Blink, "5", "25"},
    {TextAttribute::Conceal, "8", "28"},
    {TextAttribute::Strikethrough, "9", "29"},
};

// Terminal style state tracked across cells within one output stream.
struct StyleState {
    bool known = false;
    uint32_t fg = 0;        // mapped colours, 0 = default
    uint32_t bg = 0;
    uint8_t attributes = 0;
};

// Emits the minimal SGR sequence moving `state` to the cell's style. Parameter
// order: foreground, background, removed attributes, added attributes.
inline void appendStyleTransition(std::string &out, const Cell &cell, StyleState &state,
                                  ColorDepth depth)
{
    if (!state.known) {
        out += "\033[0m";
        state = StyleState{};
        state.known = true;
    }

    // Common case: same style as the previous cell.
    const uint32_t fg = mappedColor(normalizedColor(cell.fgColor), depth);
    const uint32_t bg = mappedColor(normalizedColor(cell.bgColor), depth);
    const uint8_t next = static_cast<uint8_t>(cell.attributes);
    if (fg == state.fg && bg == state.bg && next == state.attributes)
        return;

    const size_t start = out.size();
    out += "\033[";
    bool any = false;
    const auto separator = [&] {
        if (any)
            out += ';';
        any = true;
    };

    if (fg != state.fg) {
        separator();
        appendColorParameter(out, fg, depth, false);
        state.fg = fg;
    }
    if (bg != state.bg) {
        separator();
        appendColorParameter(out, bg, depth, true);
        state.bg = bg;
    }

    const uint8_t removed = state.attributes & ~next;
    uint8_t added = next & ~state.attributes;
    // SGR 22 clears both bold and faint; restore whichever remains enabled.
    const uint8_t intensity = static_cast<uint8_t>(TextAttribute::Bold)
        | static_cast<uint8_t>(TextAttribute::Dim);
    if (removed & intensity)
        added |= next & intensity;
    for (const AttributeCode &code : attributeCodes) {
        if (removed & static_cast<uint8_t>(code.attribute)) {
            separator();
            out += code.off;
        }
    }
    for (const AttributeCode &code : attributeCodes) {
        if (added & static_cast<uint8_t>(code.attribute)) {
            separator();
            out += code.on;
        }
    }
    state.attributes = next;

    if (any)
        out += 'm';
    else
        out.resize(start);
}

// Resets background and attributes (not the foreground) so that erase-to-end-
// of-line fills with the terminal default.
inline void appendEraseStyle(std::string &out, StyleState &state)
{
    if (!state.known) {
        out += "\033[0m";
        state = StyleState{};
        state.known = true;
        return;
    }
    if (state.bg == 0 && state.attributes == 0)
        return;
    out += "\033[";
    bool any = false;
    if (state.bg != 0) {
        out += "49";
        any = true;
        state.bg = 0;
    }
    for (const AttributeCode &code : attributeCodes) {
        if (state.attributes & static_cast<uint8_t>(code.attribute)) {
            if (any)
                out += ';';
            out += code.off;
            any = true;
        }
    }
    state.attributes = 0;
    out += 'm';
}

inline void appendGlyph(std::string &out, const Cell &cell)
{
    const QByteArrayView glyph = cell.character();
    if (glyph.isEmpty())
        out += ' ';
    else
        out.append(glyph.data(), static_cast<size_t>(glyph.size()));
}

// Full style as independent sequences (reset, fg, bg, one per attribute), the
// format InlineRenderer has always used for prompt rows.
inline void appendFullStyle(std::string &out, const Cell &cell, ColorDepth depth)
{
    out += "\033[0m";
    const uint32_t fg = mappedColor(normalizedColor(cell.fgColor), depth);
    if (fg != 0) {
        out += "\033[";
        appendColorParameter(out, fg, depth, false);
        out += 'm';
    }
    const uint32_t bg = mappedColor(normalizedColor(cell.bgColor), depth);
    if (bg != 0) {
        out += "\033[";
        appendColorParameter(out, bg, depth, true);
        out += 'm';
    }
    const auto attributes = static_cast<uint8_t>(cell.attributes);
    for (const AttributeCode &code : attributeCodes) {
        if (attributes & static_cast<uint8_t>(code.attribute)) {
            out += "\033[";
            out += code.on;
            out += 'm';
        }
    }
}

} // namespace qamui::ansi
