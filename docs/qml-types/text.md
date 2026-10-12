---
title: Text
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Text
category: display
summary: Displays terminal text with inline styling and explicit line breaks.
---

# Text QML type

Displays terminal text with inline styling and explicit line breaks. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `spans` | `list<span>` | `[]` | Literal styled runs; a nonempty list takes precedence over `text`. |
| `text` | `string` | `""` | Holds the plain, rich, or ANSI text to display when `spans` is empty. |
| `color` | `rgba` | `"default"` | Holds the text foreground color. |
| `bold` | `bool` | `false` | Whether text is rendered in bold face. |
| `dim` | `bool` | `false` | Whether text is rendered in dim or faint face. |
| `italic` | `bool` | `false` | Whether text is rendered in italic face if supported by the terminal. |
| `underline` | `bool` | `false` | Whether text is rendered with an underline. |
| `reversed` | `bool` | `false` | Whether foreground and background colors are inverted. |
| `horizontalAlignment` | `int` | `Qt.AlignLeft` | Holds horizontal alignment: `Qt.AlignLeft`, `Qt.AlignHCenter`, or `Qt.AlignRight`. |
| `verticalAlignment` | `int` | `Qt.AlignTop` | Holds vertical alignment: `Qt.AlignTop`, `Qt.AlignVCenter`, or `Qt.AlignBottom`. |
| `textFormat` | `TextFormat` | `Text.AutoText` | Holds how text is parsed: `Text.PlainText`, `Text.RichText`, `Text.AnsiText`, or `Text.AutoText`. |
| `gradient` | `Gradient` | `null` | Holds an optional [Gradient](gradient.md) applied across the rendered text characters. |

## Enumeration Text.TextFormat

| Value | Description |
| :--- | :--- |
| `Text.PlainText` | Text is treated as plain literal text without formatting interpretation. |
| `Text.RichText` | Basic HTML tags (such as `<b>`, `<i>`, `<font color="...">`) are parsed. |
| `Text.AnsiText` | Embedded ANSI/VT SGR escape sequences are parsed and styled. |
| `Text.AutoText` | Automatically determines format based on presence of HTML tags or ANSI escape codes. |

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
import Qamui

Terminal {
    Text {
        color: "cyan"
        bold: true
        spans: [
            { text: "Status: ", dim: true, bold: false },
            Qamui.span("Active\n", { color: "green" }),
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

Use `qamui::Text` from `<Qamui/Text.h>`. Construct it with an optional parent
`QObject`, set properties directly, and use `paint()` during the text grid
render pass.
