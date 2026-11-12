#include <QmlTui/Core/Primitives/Cell.h>

#include <QVarLengthArray>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace qmltui {

namespace {

constexpr uint32_t InvalidInternIndex = UINT32_MAX;

// Clusters longer than Cell::InlineGlyphCapacity, interned once per distinct
// byte sequence up to Cell::MaxInternedGlyphs. std::deque keeps element
// addresses stable so the views handed out by Cell::character() stay valid for
// the life of the process.
struct GlyphTable {
    std::mutex mutex;
    std::deque<std::string> glyphs;
    std::unordered_map<std::string_view, uint32_t> indices;
};

GlyphTable& glyphTable() {
    static GlyphTable table;
    return table;
}

size_t validInlineUtf8Prefix(QByteArrayView utf8) noexcept {
    if (utf8.size() <= Cell::InlineGlyphCapacity) {
        return static_cast<size_t>(utf8.size());
    }
    // If the first overflow byte is not a UTF-8 continuation byte (10xxxxxx),
    // the full InlineGlyphCapacity prefix already ends on a codepoint boundary.
    if ((static_cast<uint8_t>(utf8[Cell::InlineGlyphCapacity]) & 0xC0u) != 0x80u) {
        return static_cast<size_t>(Cell::InlineGlyphCapacity);
    }
    int pos = Cell::InlineGlyphCapacity - 1;
    while (pos > 0 && (static_cast<uint8_t>(utf8[pos]) & 0xC0u) == 0x80u) {
        --pos;
    }
    if ((static_cast<uint8_t>(utf8[pos]) & 0xC0u) == 0x80u) {
        return 0;
    }
    return static_cast<size_t>(pos);
}

} // namespace

void Cell::setLongCharacter(QByteArrayView utf8) {
    const uint32_t index = internGlyph(utf8);
    if (index != InvalidInternIndex) {
        std::memset(m_glyph, 0, sizeof m_glyph);
        std::memcpy(m_glyph, &index, sizeof index);
        m_glyphLength = InternedTag;
        return;
    }
    const size_t length = validInlineUtf8Prefix(utf8);
    if (length > 0) std::memmove(m_glyph, utf8.data(), length);
    std::memset(m_glyph + length, 0, sizeof m_glyph - length);
    m_glyphLength = static_cast<uint8_t>(length);
}

uint32_t Cell::internGlyph(QByteArrayView utf8) {
    GlyphTable& table = glyphTable();
    const std::lock_guard<std::mutex> lock(table.mutex);
    const std::string_view key(utf8.data(), static_cast<size_t>(utf8.size()));
    if (const auto it = table.indices.find(key); it != table.indices.end()) return it->second;
    if (table.glyphs.size() >= MaxInternedGlyphs) return InvalidInternIndex;
    table.glyphs.emplace_back(key);
    const auto index = static_cast<uint32_t>(table.glyphs.size() - 1);
    table.indices.emplace(std::string_view(table.glyphs.back()), index);
    return index;
}

QByteArrayView Cell::internedGlyph(uint32_t index) noexcept {
    GlyphTable& table = glyphTable();
    const std::lock_guard<std::mutex> lock(table.mutex);
    if (index >= table.glyphs.size()) return {};
    const std::string& glyph = table.glyphs[index];
    return QByteArrayView(glyph.data(), static_cast<qsizetype>(glyph.size()));
}

void Cell::appendCharacter(QByteArrayView utf8) {
    if (utf8.isEmpty()) return;
    const QByteArrayView current = character();
    if (current.size() + utf8.size() > MaxGraphemeBytes) return;
    QVarLengthArray<char, 64> joined;
    joined.append(current.data(), current.size());
    joined.append(utf8.data(), utf8.size());
    setCharacter(QByteArrayView(joined.constData(), joined.size()));
}

} // namespace qmltui
