---
title: Text
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Text
category: display
summary: Displays terminal text with inline styling and explicit line breaks.
---

# Text QML type

Displays terminal text with inline styling and explicit line breaks. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `spans` | `list<span>` | `[]` | Literal styled runs; a nonempty list takes precedence over `text`. |
| `text` | `string` | `""` | Holds the plain, rich, or ANSI text to display when `spans` is empty. |
| `color` | `color` | `"default"` | Holds the text foreground color. |
| `backgroundColor` | `color` | `"transparent"` | Holds the background color of character cells. |
| `bold` | `bool` | `false` | Whether text is rendered in bold face. |
| `dim` | `bool` | `false` | Whether text is rendered in dim or faint face. |
| `italic` | `bool` | `false` | Whether text is rendered in italic face if supported by the terminal. |
| `underline` | `bool` | `false` | Whether text is rendered with an underline. |
| `reversed` | `bool` | `false` | Whether foreground and background colors are inverted. |
| `horizontalAlignment` | `int` | `Qt.AlignLeft` | Holds horizontal alignment: `Qt.AlignLeft`, `Qt.AlignHCenter`, or `Qt.AlignRight`. |
| `verticalAlignment` | `int` | `Qt.AlignTop` | Holds vertical alignment: `Qt.AlignTop`, `Qt.AlignVCenter`, or `Qt.AlignBottom`. |
| `textFormat` | `TextFormat` | `Text.AutoText` | Holds how text is parsed: `Text.PlainText`, `Text.RichText`, `Text.AnsiText`, or `Text.AutoText`. |
| `wrapMode` | `WrapMode` | `Text.NoWrap` | Holds text wrapping mode: `Text.NoWrap`, `Text.WordWrap`, `Text.WrapAnywhere`, or `Text.Wrap`. |
| `gradient` | `Gradient` | `null` | Holds an optional [Gradient](gradient.md) applied across the rendered text characters. |

## Signals

| Signal | Description |
| :--- | :--- |
| `linkActivated(string link)` | Emitted when an embedded hyperlink tag or markdown link is clicked. |

## Enumeration Text.TextFormat

| Value | Description |
| :--- | :--- |
| `Text.PlainText` | Text is treated as plain literal text without formatting interpretation. |
| `Text.RichText` | Parses basic HTML tags (`<b>`, `<strong>`, `<i>`, `<em>`, `<u>`, `<reversed>`, `<font color="...">`, `<a href="...">`) and inline Markdown (`**bold**`, `` `code` ``, `[label](url)`). |
| `Text.AnsiText` | Embedded ANSI/VT SGR escape sequences are parsed and styled. |
| `Text.AutoText` | Parses both `RichText` (HTML tags and inline Markdown `**bold**`, `` `code` ``, `[label](url)`) and `AnsiText` (ANSI SGR escape codes). |

> [!WARNING]
> **Differences from Qt Quick**: Unlike Qt Quick's `Text.AutoText`, QmlTui's default `Text.AutoText` interprets inline Markdown (`**...**`, `` `...` ``, `[label](url)`) and ANSI SGR escape sequences. Always set `textFormat: Text.PlainText` (or use literal `spans`) when displaying user data, logs, file paths, shell commands, or source code.

## Enumeration Text.WrapMode

| Value | Description |
| :--- | :--- |
| `Text.NoWrap` | Text is not wrapped; explicit line breaks are preserved. |
| `Text.WordWrap` | Text is wrapped at word boundaries when width is exceeded. |
| `Text.WrapAnywhere` | Text is wrapped at any character boundary when width is exceeded. |
| `Text.Wrap` | Text is wrapped at word boundaries when possible, or at any character boundary. |

## Detailed Description

`Text` caches glyphs and line layouts. Newlines start lines; content does not
wrap automatically by default (`Text.NoWrap`), and wraps when `wrapMode` is set
to `Text.WordWrap`, `Text.WrapAnywhere`, or `Text.Wrap`. `implicitWidth` is the widest line in terminal cells and
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

<<< ../../examples/gallery/examples/Text.qml{qml}

### Gallery Example

![Text recorded demo](../assets/casts/components/text.gif)

See [`examples/gallery/examples/Text.qml`](../../examples/gallery/examples/Text.qml) (`./build/examples/gallery/qmltui_gallery --example Text`).
