#include <QmlTui/Core/Primitives/UnicodeText.h>

#include <algorithm>
#include <cstring>
#include <utf8proc.h>

namespace qmltui {

namespace {

// Decodes the code point at position and advances past it. A lone surrogate
// becomes '?', which is what QString::toUtf8 produced for the previous
// UTF-8-based implementation, so cluster counts and widths are unchanged.
inline char32_t decodeAt(QStringView text, qsizetype& position) noexcept {
    const char16_t first = text[position].unicode();
    ++position;
    if (QChar::isHighSurrogate(first) && position < text.size() &&
        QChar::isLowSurrogate(text[position].unicode())) {
        const char16_t second = text[position].unicode();
        ++position;
        return QChar::surrogateToUcs4(first, second);
    }
    if (QChar::isSurrogate(first)) return U'?';
    return first;
}

// Same rules as the previous clusterWidth(): the widest code point wins,
// emoji presentation, regional indicators and keycaps force two columns.
struct WidthAccumulator {
    int width = 0;
    bool emojiPresentation = false;
    bool regionalIndicator = false;
    bool keycap = false;

    void add(char32_t codePoint) noexcept {
        width = (std::max)(width, utf8proc_charwidth(static_cast<utf8proc_int32_t>(codePoint)));
        emojiPresentation |= codePoint == 0xFE0F;
        regionalIndicator |= codePoint >= 0x1F1E6 && codePoint <= 0x1F1FF;
        keycap |= codePoint == 0x20E3;
    }

    int result() const noexcept {
        if (emojiPresentation || regionalIndicator || keycap) return 2;
        return std::clamp(width, 0, 2);
    }
};

} // namespace

int UnicodeText::encodeCodePointUtf8(char32_t codePoint, char out[4]) noexcept {
    if (codePoint < 0x80) {
        out[0] = static_cast<char>(codePoint);
        return 1;
    }
    if (codePoint < 0x800) {
        out[0] = static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
        out[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 2;
    }
    if (codePoint < 0x10000) {
        if (codePoint >= 0xD800 && codePoint <= 0xDFFF) return 0;
        out[0] = static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F));
        out[1] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 3;
    }
    if (codePoint <= 0x10FFFF) {
        out[0] = static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07));
        out[1] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 4;
    }
    return 0;
}

char32_t UnicodeText::sanitizeCodePoint(char32_t codePoint) noexcept {
    if (codePoint <= 0x1f) return 0x2400 + codePoint;
    if (codePoint == 0x7f) return 0x2421;
    if (codePoint >= 0x80 && codePoint <= 0x9f) return 0xfffd;
    return codePoint;
}

GraphemeIterator::GraphemeIterator(QStringView text, bool sanitizeControls) noexcept
    : m_text(text), m_sanitizeControls(sanitizeControls) {}

bool GraphemeIterator::next(GraphemeRef& cluster) noexcept {
    if (m_position >= m_text.size()) return false;

    const qsizetype start = m_position;
    const char16_t u0 = m_text[start].unicode();
    if (u0 >= 0x20 && u0 < 0x7F) {
        if (start + 1 == m_text.size() ||
            (m_text[start + 1].unicode() >= 0x20 && m_text[start + 1].unicode() < 0x7F) ||
            m_text[start + 1].unicode() == u'\n') {
            cluster.utf16 = m_text.sliced(start, 1);
            cluster.utf16Start = static_cast<int>(start);
            cluster.width = 1;
            m_position = start + 1;
            return true;
        }
    }

    qsizetype position = m_position;
    char32_t previous = decodeAt(m_text, position);
    if (m_sanitizeControls) previous = UnicodeText::sanitizeCodePoint(previous);

    WidthAccumulator accumulator;
    accumulator.add(previous);

    utf8proc_int32_t state = 0;
    while (position < m_text.size()) {
        qsizetype nextPosition = position;
        char32_t codePoint = decodeAt(m_text, nextPosition);
        if (m_sanitizeControls) codePoint = UnicodeText::sanitizeCodePoint(codePoint);
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

int UnicodeText::encodeUtf8(QStringView cluster, char* out, int capacity,
                            bool sanitizeControls) noexcept {
    if (cluster.size() == 1 && capacity >= 1) {
        const char16_t u0 = cluster[0].unicode();
        if ((u0 >= 0x20 && u0 < 0x7F) || (!sanitizeControls && u0 < 0x80)) {
            out[0] = static_cast<char>(u0);
            return 1;
        }
    }

    int length = 0;
    qsizetype position = 0;
    while (position < cluster.size()) {
        char32_t codePoint = decodeAt(cluster, position);
        if (sanitizeControls) codePoint = sanitizeCodePoint(codePoint);
        char bytes[4];
        const int count = encodeCodePointUtf8(codePoint, bytes);
        if (length + count > capacity) return -1;
        std::memcpy(out + length, bytes, static_cast<size_t>(count));
        length += count;
    }
    return length;
}

QByteArray UnicodeText::toUtf8(QStringView cluster, bool sanitizeControls) {
    QByteArray result;
    result.reserve(cluster.size() * 4);
    qsizetype position = 0;
    while (position < cluster.size()) {
        char32_t codePoint = decodeAt(cluster, position);
        if (sanitizeControls) codePoint = sanitizeCodePoint(codePoint);
        char bytes[4];
        result.append(bytes, encodeCodePointUtf8(codePoint, bytes));
    }
    return result;
}

int UnicodeText::displayWidth(QStringView text) {
    int width = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster))
        width += cluster.width;
    return width;
}

QString UnicodeText::elideText(QStringView text, int maxColumns) {
    if (maxColumns <= 0 || text.isEmpty()) return QString();
    int width = 0;
    qsizetype endPos = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        if (width + cluster.width > maxColumns) break;
        width += cluster.width;
        endPos = cluster.utf16Start + cluster.utf16Length();
    }
    return text.left(endPos).toString();
}

int UnicodeText::nextBoundary(QStringView text, int utf16Position) {
    const qsizetype position = std::clamp<qsizetype>(utf16Position, 0, text.size());
    const qsizetype lineStart =
        position > 0 ? text.lastIndexOf(u'\n', (std::max)(qsizetype(0), position - 1)) + 1 : 0;
    GraphemeIterator iterator(text.sliced(lineStart));
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        const qsizetype end = lineStart + cluster.utf16Start + cluster.utf16Length();
        if (end > position) return static_cast<int>(end);
    }
    return static_cast<int>(text.size());
}

int UnicodeText::previousBoundary(QStringView text, int utf16Position) {
    const qsizetype position = std::clamp<qsizetype>(utf16Position, 0, text.size());
    if (position <= 0) return 0;
    const qsizetype lineStart = position >= 2 ? text.lastIndexOf(u'\n', position - 2) + 1 : 0;
    qsizetype previous = lineStart;
    GraphemeIterator iterator(text.sliced(lineStart));
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        const qsizetype start = lineStart + cluster.utf16Start;
        if (start >= position) break;
        previous = start;
    }
    return static_cast<int>(previous);
}

} // namespace qmltui
