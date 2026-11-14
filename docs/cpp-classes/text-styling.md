# TextStyle, Span, and HighlightRange

`qmltui::TextStyle`, `qmltui::Span`, and `qmltui::HighlightRange` represent sparse character-level color and SGR attribute overlays for [`Text`](text.md), [`TableView`](../qml-types/tableview.md), [`TreeView`](../qml-types/treeview.md), and [`TextArea`](../qml-types/textarea.md).

- **Headers**:
  - `<QmlTui/Core/Primitives/TextStyle.h>`
  - `<QmlTui/Core/Primitives/Span.h>`
  - `<QmlTui/Core/Primitives/HighlightRange.h>`
- **Target**: `qmltui::core` (`QmlTui::Core`)

## `qmltui::TextStyle`

`TextStyle` tracks both attribute values (`bold`, `dim`, `italic`, `underline`, `reversed`) and an explicit specification bitmask (`hasBold()`, `hasDim()`, `hasItalic()`, `hasUnderline()`, `hasReversed()`). Unset attributes inherit from the owning item or underlying highlight layer, whereas an attribute explicitly set to `false` clears an inherited attribute.

| Method | Description |
| :--- | :--- |
| `Color color() const` / `void setColor(Color)` / `void resetColor()` | Foreground color override (`Color::Invalid` when unset). |
| `Color backgroundColor() const` / `void setBackgroundColor(Color)` / `void resetBackgroundColor()` | Background color override (`Color::Invalid` when unset). |
| `bool <attr>() const` / `bool has<Attr>() const` / `void set<Attr>(bool)` / `void reset<Attr>()` | Accessors for `bold`, `dim`, `italic`, `underline`, and `reversed`. |
| `bool isEmpty() const` | Returns `true` when no color or attribute override is specified. |
| `TextAttribute resolveAttributes(TextAttribute base) const` | Applies specified attribute overrides onto `base`. |
| `uint32_t resolveForeground(uint32_t baseFg) const` / `resolveBackground(uint32_t baseBg) const` | Returns the override ARGB32 color when valid and non-transparent, otherwise `baseFg` / `baseBg`. |
| `void applyOverlay(const TextStyle& overlay)` | Overwrites only the colors and attributes explicitly specified by `overlay`. |
| `void applyVariantMap(const QVariantMap& map, bool warnOnInvalid = false)` | Populates overrides from `{ color, backgroundColor, bold, dim, italic, underline, reversed }`. |

## `qmltui::Span` (`QML_VALUE_TYPE(span)`)

`Span` pairs a literal `QString` run with a `TextStyle` without HTML or ANSI parsing. Registered in QML as the [`span`](../qml-types/span.md) structured value type and constructible via [`QmlTui.span()`](../qml-types/qmltui.md).

- **Constructors**:
  - `Span()`
  - `explicit Span(QString text)`
  - `Span(QString text, Color foreground, Color background = Color::Invalid)`
  - `Span(QString text, TextStyle style)`
- **Key Helpers**:
  - `static bool fromVariantMap(const QVariantMap& map, Span& out)`
  - `static bool tryExtractList(const QVariant& data, bool allowVariantMap, QList<Span>& outSpans)`
  - `static QString plainText(const QList<Span>& spans)`

## `qmltui::HighlightRange` (`QML_VALUE_TYPE(highlightRange)`)

`HighlightRange` pairs a UTF-16 `[start, start + length)` range with a `TextStyle` for [`TextArea::setHighlightLayer()`](../qml-types/textarea.md). Registered in QML as the [`highlightRange`](../qml-types/highlightrange.md) structured value type.

- **Constructors**:
  - `HighlightRange()`
  - `HighlightRange(int start, int length, TextStyle style = {})`
  - `HighlightRange(int start, int length, Color foreground, Color background = Color::Invalid)`
- **Properties**: `start` (`int`), `length` (`int`), `color`, `backgroundColor`, `bold`, `dim`, `italic`, `underline`, `reversed`.

```cpp
#include <QmlTui/Core/Primitives/HighlightRange.h>
#include <QmlTui/Core/Primitives/Span.h>

qmltui::TextStyle keywordStyle(qmltui::Color(QStringLiteral("cyan")));
keywordStyle.setBold(true);

qmltui::Span badge(QStringLiteral("[OK]"), keywordStyle);
qmltui::HighlightRange tokenRange(0, 6, keywordStyle);
```
