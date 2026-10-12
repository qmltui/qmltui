---
title: Paragraph
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Paragraph
category: display
summary: Displays word-wrapped rich text with clickable hyperlink support and scrolling.
---

# Paragraph QML type

Displays word-wrapped rich text with clickable hyperlink support and scrolling. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the multi-line formatted text content. |
| `textFormat` | `TextFormat` | `Paragraph.AutoText` | Holds how text is parsed: `Paragraph.AutoText`, `Paragraph.PlainText`, or `Paragraph.RichText`. |
| `color` | `rgba` | `"default"` | Holds the default text foreground color. |
| `backgroundColor` | `rgba` | `"transparent"` | Holds the paragraph background color. |
| `alignment` | `string` | `"left"` | Holds horizontal alignment: `"left"`, `"center"`, or `"right"`. |
| `wrapMode` | `bool` | `true` | Whether long text lines automatically wrap to fit the available width. |
| `scrollY` | `int` | `0` | Holds the vertical scroll offset in rows. |

## Signals

| Signal | Description |
| :--- | :--- |
| `linkActivated(string link)` | Emitted when an embedded hyperlink tag is clicked. |

## Enumeration Paragraph.TextFormat

| Value | Description |
| :--- | :--- |
| `Paragraph.AutoText` | Automatically detects HTML formatting markup. |
| `Paragraph.PlainText` | Treats text strictly as plain text without parsing markup. |
| `Paragraph.RichText` | Parses HTML formatting tags including `<b>`, `<i>`, `<font>`, and `<a href>`. |

## Detailed Description

Paragraph provides multi-line rich text rendering with automatic word wrapping.
Unlike [Text](text.md), which preserves explicit lines and requires manual newline
handling, Paragraph automatically wraps lines at word boundaries to fit the
item's available `width`.

When `textFormat` is `RichText` or `AutoText`, basic HTML markup tags are supported:
`<b>` for bold, `<i>` for italics, `<font color="...">` for colored text runs,
and `<a href="...">` for clickable hyperlinks. Clicking a hyperlink emits the
`linkActivated(link)` signal.

Vertical scrolling is supported via `scrollY`.

### Example

```qml
import QtQml
import Qamui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 40
        height: 8
        border.style: Border.Single

        Paragraph {
            anchors.fill: parent
            anchors.margins: 1
            textFormat: Paragraph.RichText
            text: "Welcome to <b>Qamui</b>! A powerful <i>retained-mode</i> terminal UI framework for Qt and C++20."
        }
    }
}
```
