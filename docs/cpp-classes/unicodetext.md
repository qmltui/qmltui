# UnicodeText & GraphemeIterator

`qmltui::UnicodeText` and `qmltui::GraphemeIterator` segment `QStringView` text into extended Unicode grapheme clusters, measure terminal column display widths (`0`, `1`, or `2` columns per cluster via `utf8proc`), elide strings to a column budget, and encode clusters to UTF-8 without heap allocation.

- **Header**: `<QmlTui/Core/Primitives/UnicodeText.h>`
- **Target**: `qmltui::core` (`QmlTui::Core`)

## `GraphemeRef` and `GraphemeIterator`

```cpp
struct GraphemeRef {
    QStringView utf16;
    int utf16Start = 0;
    int width = 1;

    int utf16Length() const noexcept;
};

class GraphemeIterator {
public:
    explicit GraphemeIterator(QStringView text, bool sanitizeControls = false) noexcept;
    bool next(GraphemeRef& cluster) noexcept;
    void reset() noexcept;
};
```

`GraphemeIterator` performs a single forward pass over `text` without allocating memory (`text` must outlive the iterator). When `sanitizeControls` is `true`, C0 controls (`0x00..0x1F`), `DEL` (`0x7F`), and C1 controls (`0x80..0x9F`) participate in cluster breaking and width calculation as their visible replacement glyphs (`U+2400 + code`, `U+2421`, and `U+FFFD`), matching what [`TextGridBuffer`](textgridbuffer.md) writes into cells.

## `UnicodeText` Static Methods

| Method | Description |
| :--- | :--- |
| `static int displayWidth(QStringView text, bool sanitizeControls = false)` | Returns the total terminal column width of `text` (`0`, `1`, or `2` columns per grapheme cluster, accounting for East Asian wide characters, emoji sequences, and combining marks). |
| `static QString elideText(QStringView text, int maxColumns)` | Truncates `text` at grapheme boundaries with a trailing ellipsis (`…`) so its terminal column width does not exceed `maxColumns`. |
| `static int nextBoundary(QStringView text, int utf16Position)` | Returns the next grapheme cluster boundary after `utf16Position` in UTF-16 code units. |
| `static int previousBoundary(QStringView text, int utf16Position)` | Returns the previous grapheme cluster boundary before `utf16Position` in UTF-16 code units. |
| `static int encodeCodePointUtf8(char32_t codePoint, char out[4]) noexcept` | Encodes one Unicode scalar value into `out` (`1..4` bytes), returning `0` for surrogates or out-of-range values. |
| `static int encodeUtf8(QStringView cluster, char* out, int capacity, bool sanitizeControls = false) noexcept` | Encodes `cluster` as UTF-8 into `out`, returning the byte count written or `-1` if `capacity` is insufficient. |
| `static QByteArray toUtf8(QStringView cluster, bool sanitizeControls = false)` | Encodes `cluster` as a `QByteArray` in UTF-8, optionally replacing control characters with visible symbols. |
| `static char32_t sanitizeCodePoint(char32_t codePoint) noexcept` | Returns the visible replacement code point used for control characters in sanitizing mode. |

## Example: Custom Cell-Width Measurement

Always measure terminal column widths with `UnicodeText::displayWidth()` rather than `QString::size()`. Precompute layout widths in property setters or `Item::updateLayout()` rather than on the hot `Item::paint()` path:

```cpp
#include <QmlTui/Core/Primitives/UnicodeText.h>

void StatusBadge::setLabel(const QString& label) {
    if (m_label == label)
        return;
    m_label = label;
    setImplicitWidth(qmltui::UnicodeText::displayWidth(m_label) + 2);
    markDirty();
}
```
