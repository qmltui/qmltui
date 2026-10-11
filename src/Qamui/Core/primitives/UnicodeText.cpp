#include <Qamui/Core/primitives/UnicodeText.h>

#include <algorithm>
#include <cstring>
#include <utf8proc.h>

namespace qamui {

namespace {

// Decodes the code point at position and advances past it. A lone surrogate
// becomes '?', which is what QString::toUtf8 produced for the previous
// UTF-8-based implementation, so cluster counts and widths are unchanged.
inline char32_t decodeAt(QStringView text, qsizetype &position) noexcept
{
    const char16_t first = text[position].unicode();
    ++position;
    if (QChar::isHighSurrogate(first) && position < text.size()
        && QChar::isLowSurrogate(text[position].unicode())) {
        const char16_t second = text[position].unicode();
        ++position;
        return QChar::surrogateToUcs4(first, second);
    }
    if (QChar::isSurrogate(first))
        return U'?';
    return first;
}

// Same rules as the previous clusterWidth(): the widest code point wins,
// emoji presentation, regional indicators and keycaps force two columns.
struct WidthAccumulator {
    int width = 0;
    bool emojiPresentation = false;
    bool regionalIndicator = false;
    bool keycap = false;

    void add(char32_t codePoint) noexcept
    {
        width = std::max(width, utf8proc_charwidth(static_cast<utf8proc_int32_t>(codePoint)));
        emojiPresentation |= codePoint == 0xFE0F || (codePoint >= 0x1F300 && codePoint <= 0x1FAFF);
        regionalIndicator |= codePoint >= 0x1F1E6 && codePoint <= 0x1F1FF;
        keycap |= codePoint == 0x20E3;
    }

    int result() const noexcept
    {
        if (emojiPresentation || regionalIndicator || keycap)
            return 2;
        return std::clamp(width, 0, 2);
    }
};

inline int encodeCodePoint(char32_t codePoint, char *out) noexcept
{
    if (codePoint < 0x80) {
        out[0] = static_cast<char>(codePoint);
        return 1;
    }
    if (codePoint < 0x800) {
        out[0] = static_cast<char>(0xC0 | (codePoint >> 6));
        out[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 2;
    }
    if (codePoint < 0x10000) {
        out[0] = static_cast<char>(0xE0 | (codePoint >> 12));
        out[1] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 3;
    }
    out[0] = static_cast<char>(0xF0 | (codePoint >> 18));
    out[1] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
    out[2] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
    out[3] = static_cast<char>(0x80 | (codePoint & 0x3F));
    return 4;
}

} // namespace

char32_t UnicodeText::sanitizeCodePoint(char32_t codePoint) noexcept
{
    if (codePoint <= 0x1f)
        return 0x2400 + codePoint;
    if (codePoint == 0x7f)
        return 0x2421;
    if (codePoint >= 0x80 && codePoint <= 0x9f)
        return 0xfffd;
    return codePoint;
}

GraphemeIterator::GraphemeIterator(QStringView text, bool sanitizeControls) noexcept
    : m_text(text), m_sanitizeControls(sanitizeControls)
{
}

bool GraphemeIterator::next(GraphemeRef &cluster) noexcept
{
    if (m_position >= m_text.size())
        return false;

    const qsizetype start = m_position;
    qsizetype position = m_position;
    char32_t previous = decodeAt(m_text, position);
    if (m_sanitizeControls)
        previous = UnicodeText::sanitizeCodePoint(previous);

    WidthAccumulator accumulator;
    accumulator.add(previous);

    utf8proc_int32_t state = 0;
    while (position < m_text.size()) {
        qsizetype nextPosition = position;
        char32_t codePoint = decodeAt(m_text, nextPosition);
        if (m_sanitizeControls)
            codePoint = UnicodeText::sanitizeCodePoint(codePoint);
        if (utf8proc_grapheme_break_stateful(static_cast<utf8proc_int32_t>(previous),
                                             static_cast<utf8proc_int32_t>(codePoint), &state))
            break;
        accumulator.add(codePoint);
        previous = codePoint;
        position = nextPosition;
    }

    cluster.utf16 = m_text.mid(start, position - start);
    cluster.utf16Start = static_cast<int>(start);
    cluster.width = accumulator.result();
    m_position = position;
    return true;
}

int UnicodeText::encodeUtf8(QStringView cluster, char *out, int capacity,
                            bool sanitizeControls) noexcept
{
    int length = 0;
    qsizetype position = 0;
    while (position < cluster.size()) {
        char32_t codePoint = decodeAt(cluster, position);
        if (sanitizeControls)
            codePoint = sanitizeCodePoint(codePoint);
        char bytes[4];
        const int count = encodeCodePoint(codePoint, bytes);
        if (length + count > capacity)
            return -1;
        std::memcpy(out + length, bytes, static_cast<size_t>(count));
        length += count;
    }
    return length;
}

QByteArray UnicodeText::toUtf8(QStringView cluster, bool sanitizeControls)
{
    QByteArray result;
    result.reserve(cluster.size() * 4);
    qsizetype position = 0;
    while (position < cluster.size()) {
        char32_t codePoint = decodeAt(cluster, position);
        if (sanitizeControls)
            codePoint = sanitizeCodePoint(codePoint);
        char bytes[4];
        result.append(bytes, encodeCodePoint(codePoint, bytes));
    }
    return result;
}

QVector<Grapheme> UnicodeText::graphemes(QStringView text)
{
    QVector<Grapheme> result;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        Grapheme grapheme;
        grapheme.utf8 = toUtf8(cluster.utf16);
        grapheme.utf16Start = cluster.utf16Start;
        grapheme.utf16Length = cluster.utf16Length();
        grapheme.width = cluster.width;
        result.append(grapheme);
    }
    return result;
}

int UnicodeText::displayWidth(QStringView text)
{
    int width = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster))
        width += cluster.width;
    return width;
}

int UnicodeText::nextBoundary(QStringView text, int utf16Position)
{
    const int position = std::clamp(utf16Position, 0, static_cast<int>(text.size()));
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        const int end = cluster.utf16Start + cluster.utf16Length();
        if (end > position)
            return end;
    }
    return static_cast<int>(text.size());
}

int UnicodeText::previousBoundary(QStringView text, int utf16Position)
{
    const int position = std::clamp(utf16Position, 0, static_cast<int>(text.size()));
    int previous = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        if (cluster.utf16Start >= position)
            break;
        previous = cluster.utf16Start;
    }
    return previous;
}

} // namespace qamui
