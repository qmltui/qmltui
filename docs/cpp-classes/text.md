# Text

## 1. Class Overview

`Text` renders single-line or multi-line formatted text strings in the terminal. It supports bold, dim, italic, underline, reversed video attributes, horizontal/vertical alignment, and text format modes (`PlainText`, `RichText` HTML tag parsing, `AnsiText` escape sequences, and `AutoText`).

Developers use `Text` for labels, headlines, and data output in terminal UIs.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/items/Text.h`
- **Implementation**: `Tui/src/items/Text.cpp`
- **Dependencies**: `Item.h`, `TextGridBuffer.h`, `Color.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits visual placement, alignment, and clipping functionality.

## 4. Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `text` | `QString` | `text` | `setText` | `textChanged` | Text content string to display. |
| `color` | `qmltui::Color` | `color` | `setColor` | `colorChanged` | Foreground text color; defaults to the terminal foreground. |
| `bold` | `bool` | `bold` | `setBold` | `boldChanged` | Renders text in bold attribute. |
| `dim` | `bool` | `dim` | `setDim` | `dimChanged` | Uses faint intensity (SGR 2). Disabled text is also dimmed, preserving its color. |
| `italic` | `bool` | `italic` | `setItalic` | `italicChanged` | Renders text in italic attribute. |
| `underline` | `bool` | `underline` | `setUnderline` | `underlineChanged` | Renders text with underline. |
| `reversed` | `bool` | `reversed` | `setReversed` | `reversedChanged` | Inverts foreground and background colors. |
| `horizontalAlignment` | `int` | `horizontalAlignment` | `setHorizontalAlignment` | `horizontalAlignmentChanged` | AlignLeft, AlignHCenter, or AlignRight. |
| `verticalAlignment` | `int` | `verticalAlignment` | `setVerticalAlignment` | `verticalAlignmentChanged` | AlignTop, AlignVCenter, or AlignBottom. |
| `textFormat` | `TextFormat` | `textFormat` | `setTextFormat` | `textFormatChanged` | PlainText, RichText, AnsiText, or AutoText. |

## 5. Enumerations

### TextFormat

| Value | Integer | Description |
|-------|---------|-------------|
| `PlainText` | `0` | Displays raw text without parsing HTML tags or ANSI escape sequences. |
| `RichText` | `1` | Parses HTML formatting tags like `<b>`, `<i>`, `<font color="...">`. |
| `AnsiText` | `2` | Parses raw ANSI color and attribute escape codes. |
| `AutoText` | `3` | Automatically detects HTML tags or ANSI codes and applies formatting. |

## 10. Protected Virtual Methods / Event Handlers

#### void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const [override]
Draws the formatted text line(s) into `buffer` applying alignment and font attributes.

## 16. Usage Example

```cpp
#include <QmlTui/Core/Items/Text.h>

using namespace qmltui;

Text* textItem = new Text(parent);
textItem->setText("Hello <b>World</b>");
textItem->setTextFormat(Text::RichText);
textItem->setColor("green");
```

See the [QML type reference](../qml-types/text.md) for QML usage.
