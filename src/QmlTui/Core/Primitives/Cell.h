#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <QByteArrayView>

#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>

namespace qmltui {

enum class TextAttribute : uint8_t {
    None      = 0,
    Bold      = 1 << 0,
    Italic    = 1 << 1,
    Underline = 1 << 2,
    Reversed  = 1 << 3,
    Blink = 1 << 4,
    Conceal = 1 << 5,
    Strikethrough = 1 << 6,
    Dim = 1 << 7
};

inline TextAttribute operator|(TextAttribute a, TextAttribute b) {
    return static_cast<TextAttribute>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TextAttribute operator&(TextAttribute a, TextAttribute b) {
    return static_cast<TextAttribute>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// One terminal grid position: 24 bytes, trivially copyable, no heap use.
//
// The grapheme cluster is stored inline when it fits in InlineGlyphCapacity
// bytes (every single code point, emoji with skin tone or variation selector,
// flags and keycaps). Longer clusters such as ZWJ sequences are interned in a
// process-wide append-only table and referenced by index, so cells still
// compare and copy by value across buffers and the table is never freed.
struct QMLTUI_CORE_EXPORT Cell {
    static constexpr int InlineGlyphCapacity = 11;

    uint32_t fgColor = 0x00000000; // Terminal default foreground
    uint32_t bgColor = 0x00000000; // Default background (Transparent)
    uint8_t displayWidth = 1;
    bool continuation = false;
    TextAttribute attributes = TextAttribute::None;
    uint8_t linkId = 0;

    // UTF-8 bytes of the cluster (empty for continuation cells). The view is
    // valid until this cell is modified or its buffer is resized, cleared,
    // swapped or destroyed; views of interned clusters live for the process.
    QByteArrayView character() const noexcept
    {
        if (m_glyphLength != InternedTag)
            return QByteArrayView(m_glyph, m_glyphLength);
        return internedGlyph(internIndex());
    }

    // Replaces the cluster. Accepts literals, QByteArray and views, including
    // a view of this cell's own bytes.
    void setCharacter(QByteArrayView utf8)
    {
        if (utf8.size() <= InlineGlyphCapacity) {
            const auto length = static_cast<size_t>(utf8.size());
            if (length > 0)
                std::memmove(m_glyph, utf8.data(), length);
            std::memset(m_glyph + length, 0, sizeof m_glyph - length);
            m_glyphLength = static_cast<uint8_t>(length);
        } else {
            const uint32_t index = internGlyph(utf8);
            std::memset(m_glyph, 0, sizeof m_glyph);
            std::memcpy(m_glyph, &index, sizeof index);
            m_glyphLength = InternedTag;
        }
    }

    // Appends bytes to the cluster (combining marks, variation selectors).
    void appendCharacter(QByteArrayView utf8);

    void clearCharacter() noexcept { setCharacter(QByteArrayView()); }
    bool hasCharacter() const noexcept { return m_glyphLength != 0; }

    void setForeground(Color color) { fgColor = color.paintedArgb32(); }
    void setBackground(Color color) { bgColor = color.paintedArgb32(); }
    Color foreground() const { return Color::fromArgb32(fgColor); }
    Color background() const { return Color::fromArgb32(bgColor); }

    // Unused inline bytes are always zero, so a memberwise compare is exact.
    bool operator==(const Cell &other) const noexcept = default;

private:
    static constexpr uint8_t InternedTag = 0xFF;

    static uint32_t internGlyph(QByteArrayView utf8);
    static QByteArrayView internedGlyph(uint32_t index) noexcept;

    uint32_t internIndex() const noexcept
    {
        uint32_t index = 0;
        std::memcpy(&index, m_glyph, sizeof index);
        return index;
    }

    uint8_t m_glyphLength = 1;            // 0..InlineGlyphCapacity, or InternedTag
    char m_glyph[InlineGlyphCapacity] = {' '};  // interned: bytes 0..3 hold the index
};

static_assert(sizeof(Cell) == 24, "Cell must stay 24 bytes");
static_assert(std::is_trivially_copyable_v<Cell>, "Cell must be trivially copyable");

} // namespace qmltui
