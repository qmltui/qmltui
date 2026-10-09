---
title: Text
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::Text
category: display
summary: Displays terminal text with inline styling and explicit line breaks.
---

# Text QML type

Displays terminal text with inline styling and explicit line breaks. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `spans` | `list<span>` | `[]` | Literal styled runs; a nonempty list takes precedence over `text`. |
| `text` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `bold` | `bool` |  | _TODO_ |
| `dim` | `bool` |  | _TODO_ |
| `italic` | `bool` |  | _TODO_ |
| `underline` | `bool` |  | _TODO_ |
| `reversed` | `bool` |  | _TODO_ |
| `horizontalAlignment` | `int` |  | _TODO_ |
| `verticalAlignment` | `int` |  | _TODO_ |
| `textFormat` | `TextFormat` |  | _TODO_ |
| `gradient` | `Gradient` |  | _TODO_ |

## Enumeration Text.TextFormat

| Value | Description |
| :--- | :--- |
| `Text.PlainText` | _TODO_ |
| `Text.RichText` | _TODO_ |
| `Text.AnsiText` | _TODO_ |
| `Text.AutoText` | _TODO_ |

## Detailed Description

`Text` caches glyphs and line layouts. Newlines start lines; content does not
wrap automatically. `implicitWidth` is the widest line in terminal cells and
`implicitHeight` counts lines, including consecutive and trailing empty lines.
Horizontal alignment applies per line; vertical alignment positions the whole
block. Content outside the available dimensions is clipped.

A nonempty `spans` list renders [span](span.md) values in sequence. Adjacent
spans continue on the same line, and a span can contain multiple lines. Clearing
`spans` restores the current `text`; `[ { text: "" } ]` renders empty content
instead. Changing either property updates the implicit dimensions.

Span content is literal: `textFormat` only affects `text`, and HTML/ANSI markup
is not interpreted in spans. Control characters use the existing sanitized
Unicode rendering behavior. Graphemes are segmented across span boundaries;
a combined character or emoji uses the style of the span containing its first
UTF-16 code unit.

Omitted span colors and attributes inherit the Text foreground and attributes.
Explicit `false` disables an inherited attribute. Explicit `color: "default"`
requests terminal-default foreground even when Text has another color.
Background defaults to transparent. An active gradient overrides all glyph
foregrounds, including explicit span colors. Effective disabled state adds dim
at paint time, including to spans that explicitly set `dim: false`.

### Example

```qml
pragma ComponentBehavior: Bound
import QtQml
import Qtui

Terminal {
    Text {
        color: "cyan"
        bold: true
        spans: [
            { text: "Status: ", dim: true, bold: false },
            Qtui.span("Active\n", { color: "green" }),
            { text: "Workers: 4" }
        ]
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
}
```

### C++

```cpp
#include <Qtui/Core/items/Text.h>

qtui::Span label(QStringLiteral("Status: "));
label.setBold(false);
label.setDim(true);
qtui::Text text;
text.setSpans({label, qtui::Span(QStringLiteral("Active"), qtui::Color("green"))});
```

Spans have value semantics without per-span QObject allocation. Strings,
containers, QML construction, and layout storage can still allocate.
