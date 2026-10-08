---
title: Panel
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: overlays and navigation
summary: A bordered rectangle with an optional title drawn into its top border.
---

# Panel QML type

A bordered rectangle with an optional title drawn into its top border. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `title` | `string` | `""` | Holds the caption drawn over the top border, padded with one space on each side; an empty string draws no caption. |
| `titleAlignment` | `int` | `Qt.AlignLeft` | Holds where the caption sits on the top border: `Qt.AlignLeft`, `Qt.AlignHCenter` or `Qt.AlignRight`; any other value is treated as left. |
| `titleColor` | `rgba` | `"default"` | Holds the terminal colour of the caption, where `"default"` is the terminal's own foreground colour. |

## Detailed Description

Panel is a [Rectangle](rectangle.md) whose `border.style` defaults to
`"single"` and `border.color` to `"default"`, with a [Text](text.md) child
placed on row 0 so that the caption replaces part of the top border line. The
caption is `title` with a leading and trailing space so it stands clear of
the line characters. Left alignment starts it at column 2, centre alignment
places it at the middle of the panel's `width` (but never before column 1),
and right alignment ends it 2 cells before the right edge. The caption is
not truncated, so a title wider than the panel overruns the border.

Panel adds no layout of its own: children are positioned as in any
Rectangle, and content should be inset by one cell on each side to stay
inside the border, for example with `anchors.fill: parent` and
`anchors.margins: 1`. It has no implicit size, so set `width` and `height`
or anchor it. Change `border.style` to `"double"`, `"bold"`, `"ascii"` or
`"none"` like any Rectangle; types without a `border` group, such as
[TabBar](tabbar.md) or [TableView](tableview.md), are typically wrapped in a
Panel to get a titled frame.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Panel {
        anchors.centerIn: parent
        width: 40
        height: 8
        title: "Pod Metrics"
        titleAlignment: Qt.AlignHCenter
        titleColor: "cyan"
        border.style: "double"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Text { text: "CPU: 42%   Memory: 512 MiB" }
            Button { text: "Quit"; focus: true; onClicked: Qt.quit() }
        }
    }
}
```
