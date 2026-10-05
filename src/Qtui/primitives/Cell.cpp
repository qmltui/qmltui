#include "Cell.h"

#include <QVarLengthArray>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace qtui {

namespace {

// Clusters longer than Cell::InlineGlyphCapacity, interned once per distinct
// byte sequence. std::deque keeps element addresses stable so the views handed
// out by Cell::character() stay valid for the life of the process. The table
// only grows by the number of distinct long clusters ever displayed.
struct GlyphTable {
    std::mutex mutex;
    std::deque<std::string> glyphs;
    std::unordered_map<std::string_view, uint32_t> indices;
};

GlyphTable &glyphTable()
{
    static GlyphTable table;
    return table;
}

} // namespace

uint32_t Cell::internGlyph(QByteArrayView utf8)
{
    GlyphTable &table = glyphTable();
    const std::lock_guard<std::mutex> lock(table.mutex);
    const std::string_view key(utf8.data(), static_cast<size_t>(utf8.size()));
    if (const auto it = table.indices.find(key); it != table.indices.end())
        return it->second;
    table.glyphs.emplace_back(key);
    const auto index = static_cast<uint32_t>(table.glyphs.size() - 1);
    table.indices.emplace(std::string_view(table.glyphs.back()), index);
    return index;
}

QByteArrayView Cell::internedGlyph(uint32_t index) noexcept
{
    GlyphTable &table = glyphTable();
    const std::lock_guard<std::mutex> lock(table.mutex);
    if (index >= table.glyphs.size())
        return {};
    const std::string &glyph = table.glyphs[index];
    return QByteArrayView(glyph.data(), static_cast<qsizetype>(glyph.size()));
}

void Cell::appendCharacter(QByteArrayView utf8)
{
    if (utf8.isEmpty())
        return;
    const QByteArrayView current = character();
    QVarLengthArray<char, 64> joined;
    joined.append(current.data(), current.size());
    joined.append(utf8.data(), utf8.size());
    setCharacter(QByteArrayView(joined.constData(), joined.size()));
}

} // namespace qtui
