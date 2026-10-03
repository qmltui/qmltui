#include "UnicodeText.h"

#include <algorithm>
#include <utf8proc.h>

namespace qtui {

int UnicodeText::clusterWidth(const QVector<int> &codepoints) {
    int width = 0;
    bool emojiPresentation = false;
    bool regionalIndicator = false;
    bool keycap = false;

    for (int cp : codepoints) {
        width = std::max(width, utf8proc_charwidth(cp));
        emojiPresentation |= cp == 0xFE0F || (cp >= 0x1F300 && cp <= 0x1FAFF);
        regionalIndicator |= cp >= 0x1F1E6 && cp <= 0x1F1FF;
        keycap |= cp == 0x20E3;
    }

    if (emojiPresentation || regionalIndicator || keycap)
        return 2;
    return std::clamp(width, 0, 2);
}

QVector<Grapheme> UnicodeText::graphemes(const QString &text) {
    QVector<Grapheme> result;
    const QByteArray utf8 = text.toUtf8();
    if (utf8.isEmpty())
        return result;

    int byteOffset = 0;
    int clusterByteStart = 0;
    int clusterUtf16Start = 0;
    int utf16Offset = 0;
    int previousCodepoint = -1;
    int state = 0;
    QVector<int> clusterCodepoints;

    auto appendCluster = [&](int byteEnd, int utf16End) {
        if (byteEnd <= clusterByteStart)
            return;
        Grapheme cluster;
        cluster.utf8 = utf8.mid(clusterByteStart, byteEnd - clusterByteStart);
        cluster.utf16Start = clusterUtf16Start;
        cluster.utf16Length = utf16End - clusterUtf16Start;
        cluster.width = clusterWidth(clusterCodepoints);
        result.append(cluster);
        clusterCodepoints.clear();
        clusterByteStart = byteEnd;
        clusterUtf16Start = utf16End;
    };

    while (byteOffset < utf8.size()) {
        utf8proc_int32_t codepoint = 0;
        const auto consumed = utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t *>(utf8.constData() + byteOffset),
            utf8.size() - byteOffset, &codepoint);
        const int byteLength = consumed > 0 ? static_cast<int>(consumed) : 1;
        if (consumed <= 0)
            codepoint = 0xFFFD;

        const int codepointUtf16Length = codepoint > 0xFFFF ? 2 : 1;
        if (previousCodepoint >= 0 &&
            utf8proc_grapheme_break_stateful(previousCodepoint, codepoint, &state)) {
            appendCluster(byteOffset, utf16Offset);
            state = 0;
        }

        clusterCodepoints.append(codepoint);
        previousCodepoint = codepoint;
        byteOffset += byteLength;
        utf16Offset += codepointUtf16Length;
    }
    appendCluster(byteOffset, utf16Offset);
    return result;
}

int UnicodeText::displayWidth(const QString &text) {
    int width = 0;
    for (const Grapheme &cluster : graphemes(text))
        width += cluster.width;
    return width;
}

int UnicodeText::nextBoundary(const QString &text, int utf16Position) {
    const int position = std::clamp(utf16Position, 0, static_cast<int>(text.size()));
    for (const Grapheme &cluster : graphemes(text)) {
        const int end = cluster.utf16Start + cluster.utf16Length;
        if (end > position)
            return end;
    }
    return text.size();
}

int UnicodeText::previousBoundary(const QString &text, int utf16Position) {
    const int position = std::clamp(utf16Position, 0, static_cast<int>(text.size()));
    int previous = 0;
    for (const Grapheme &cluster : graphemes(text)) {
        if (cluster.utf16Start >= position)
            break;
        previous = cluster.utf16Start;
    }
    return previous;
}

} // namespace qtui
