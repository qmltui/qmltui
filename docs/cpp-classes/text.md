# Text

## Class Overview

`Text` renders single-line or multi-line formatted text strings or structured `Span` runs in the terminal. It supports foreground and background colors, gradients, bold, dim, italic, underline, and reversed video attributes, horizontal/vertical alignment, wrapping (`NoWrap`, `WordWrap`, `WrapAnywhere`, `Wrap`), clickable hyperlinks (`linkActivated`), and text format modes (`PlainText`, `RichText` HTML tag parsing, `AnsiText` escape sequences, and `AutoText`).

Developers use `Text` for labels, headlines, and data output in terminal UIs.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Items/Text.h` (installed as `<QmlTui/Core/Items/Text.h>`)
- **Implementation**: `src/QmlTui/Core/Items/Text.cpp`
- **Dependencies**: `Item.h`, `Span.h`, `Gradient.h`, `TextGridBuffer.h`, `Color.h`.

## Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits visual placement, alignment, and clipping functionality.

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `spans` | `QList<qmltui::Span>` | `spans` | `setSpans` | `spansChanged` | Structured text runs; when non-empty, takes precedence over `text`. |
| `text` | `QString` | `text` | `setText` | `textChanged` | Text content string to display when `spans` is empty. |
| `color` | `qmltui::Color` | `color` | `setColor` | `colorChanged` | Foreground text color; defaults to the terminal foreground (`"default"`). |
| `backgroundColor` | `qmltui::Color` | `backgroundColor` | `setBackgroundColor` | `backgroundColorChanged` | Background color of painted character cells; defaults to `"transparent"`. |
| `bold` | `bool` | `bold` | `setBold` | `boldChanged` | Renders text in bold attribute. |
| `dim` | `bool` | `dim` | `setDim` | `dimChanged` | Uses faint intensity (SGR 2). Disabled text is also dimmed, preserving its color. |
| `italic` | `bool` | `italic` | `setItalic` | `italicChanged` | Renders text in italic attribute. |
| `underline` | `bool` | `underline` | `setUnderline` | `underlineChanged` | Renders text with underline. |
| `reversed` | `bool` | `reversed` | `setReversed` | `reversedChanged` | Inverts foreground and background colors. |
| `horizontalAlignment` | `int` | `horizontalAlignment` | `setHorizontalAlignment` | `horizontalAlignmentChanged` | `Qt::AlignLeft`, `Qt::AlignHCenter`, or `Qt::AlignRight`. |
| `verticalAlignment` | `int` | `verticalAlignment` | `setVerticalAlignment` | `verticalAlignmentChanged` | `Qt::AlignTop`, `Qt::AlignVCenter`, or `Qt::AlignBottom`. |
| `textFormat` | `TextFormat` | `textFormat` | `setTextFormat` | `textFormatChanged` | `PlainText`, `RichText`, `AnsiText`, or `AutoText`. |
| `wrapMode` | `WrapMode` | `wrapMode` | `setWrapMode` | `wrapModeChanged` | `NoWrap`, `WordWrap`, `WrapAnywhere`, or `Wrap`. |
| `gradient` | `qmltui::Gradient*` | `gradient` | `setGradient` | `gradientChanged` | Optional foreground gradient applied across rendered characters. |

## Enumerations

### TextFormat

| Value | Integer | Description |
|-------|---------|-------------|
| `PlainText` | `0` | Displays raw text without parsing HTML tags or ANSI escape sequences. |
| `RichText` | `1` | Parses HTML formatting tags like `<b>`, `<i>`, `<font color="...">`, `<a href="...">`. |
| `AnsiText` | `2` | Parses raw ANSI color and attribute escape codes. |
| `AutoText` | `3` | Automatically detects HTML tags, markdown links, or ANSI codes and applies formatting. |

### WrapMode

| Value | Integer | Description |
|-------|---------|-------------|
| `NoWrap` | `0` | Does not wrap automatically; preserves explicit line breaks. |
| `WordWrap` | `1` | Wraps at word boundaries when the item width is exceeded. |
| `WrapAnywhere` | `2` | Wraps at any grapheme boundary when the item width is exceeded. |
| `Wrap` | `3` | Wraps at word boundaries when possible, falling back to grapheme boundaries. |

## Signals

#### void linkActivated(const QString& link)
Emitted when the user clicks an embedded HTML `<a href="...">` or markdown `[label](url)` hyperlink.

## Protected Virtual Methods / Event Handlers

#### void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const [override]
Draws the formatted text line(s) into `buffer` applying alignment, wrapping, hyperlinks, and font attributes.

## Usage Example

```cpp
#include <QmlTui/Core/Items/Text.h>

using namespace qmltui;

Text* textItem = new Text(parent);
textItem->setText("Hello <b>World</b>");
textItem->setTextFormat(Text::RichText);
textItem->setWrapMode(Text::WordWrap);
textItem->setColor("green");
```

See the [QML type reference](../qml-types/text.md) for QML usage.
