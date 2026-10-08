#pragma once

#include <QByteArray>
#include <QString>
#include <QStringView>
#include <QVector>

namespace qtui {

// Owning description of one grapheme cluster (legacy API; allocates).
struct Grapheme {
    QByteArray utf8;
    int utf16Start = 0;
    int utf16Length = 0;
    int width = 1;
};

// Non-owning description of one grapheme cluster inside the iterated string.
struct GraphemeRef {
    QStringView utf16;
    int utf16Start = 0;
    int width = 1;

    int utf16Length() const noexcept { return static_cast<int>(utf16.size()); }
};

// Single forward pass over the grapheme clusters of a string without heap
// allocation. The string must outlive the iterator.
//
// With sanitizeControls, C0 controls, DEL and C1 controls take part in
// cluster breaking and width as their visible replacements (U+2400 + code,
// U+2421, U+FFFD), matching what TextGridBuffer writes for them.
class GraphemeIterator {
public:
    explicit GraphemeIterator(QStringView text, bool sanitizeControls = false) noexcept;

    // Advances to the next cluster; returns false at the end of the string.
    bool next(GraphemeRef &cluster) noexcept;
    void reset() noexcept { m_position = 0; }

private:
    QStringView m_text;
    qsizetype m_position = 0;
    bool m_sanitizeControls = false;
};

class UnicodeText {
public:
    static QVector<Grapheme> graphemes(QStringView text);
    static int displayWidth(QStringView text);
    static int nextBoundary(QStringView text, int utf16Position);
    static int previousBoundary(QStringView text, int utf16Position);

    // Encodes a cluster as UTF-8 into out. Returns the byte count, or -1 when
    // capacity is insufficient (nothing is written in that case).
    static int encodeUtf8(QStringView cluster, char *out, int capacity,
                          bool sanitizeControls = false) noexcept;
    static QByteArray toUtf8(QStringView cluster, bool sanitizeControls = false);

    // Visible replacement used for control characters by the sanitising modes.
    static char32_t sanitizeCodePoint(char32_t codePoint) noexcept;
};

} // namespace qtui
