---
title: Panel
kind: qml
import: Qamui.Controls
since: "0.4"
inherits: Rectangle
cpp: qamui::Rectangle
category: overlays and navigation
summary: A bordered rectangle with optional title, badge, trailing text, and footer drawn into its borders.
---

# Panel QML type

A bordered rectangle with optional title, badge, trailing text, and footer drawn into its borders. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui.Controls` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `footer` | `string` | `""` | Holds the caption drawn on the left of the bottom border, padded with spaces; an empty string draws no footer. |
| `footerColor` | `rgba` | `"default"` | Holds the terminal colour of the left bottom footer caption. |
| `footerTrailing` | `string` | `""` | Holds the right-aligned caption drawn on the bottom border, padded with spaces. |
| `footerTrailingColor` | `rgba` | `"default"` | Holds the terminal colour of the right-aligned bottom footer caption. |
| `title` | `string` | `""` | Holds the caption drawn over the top border, padded with one space on each side; an empty string draws no caption. |
| `titleAlignment` | `int` | `Qt.AlignLeft` | Holds where the caption sits on the top border: `Qt.AlignLeft`, `Qt.AlignHCenter` or `Qt.AlignRight`; any other value is treated as left. |
| `titleBadge` | `string` | `""` | Holds an accent badge drawn immediately after the title, with its own colour. |
| `titleBadgeColor` | `rgba` | `"default"` | Holds the terminal colour of the title badge. |
| `titleColor` | `rgba` | `"default"` | Holds the terminal colour of the caption, where `"default"` is the terminal's own foreground colour. |
| `titleTrailing` | `string` | `""` | Holds the right-aligned caption drawn on the top border, padded with spaces. |
| `titleTrailingColor` | `rgba` | `"default"` | Holds the terminal colour of the right-aligned top border caption. |

## Detailed Description

Panel is a [Rectangle](rectangle.md) whose `border.style` defaults to
`"single"` and `border.color` to `"default"`, with border text slots
placed on the top and bottom borders so that captions replace parts of the
border lines.

Top border slots include:
- `title`: the primary caption, positioned by `titleAlignment` (`Qt.AlignLeft`,
  `Qt.AlignHCenter`, or `Qt.AlignRight`).
- `titleBadge`: an optional label placed immediately after `title` with its
  own `titleBadgeColor`.
- `titleTrailing`: right-aligned text ending 2 cells before the right edge.
  When `titleAlignment` is centered, the title and badge are clamped so they
  do not collide with trailing text.

Bottom border slots include:
- `footer`: left-aligned text on the bottom border.
- `footerTrailing`: right-aligned text on the bottom border.

Each caption is padded with spaces so it stands clear of the line characters.
Panel adds no layout of its own: children are positioned as in any
Rectangle, and content should be inset by one cell on each side to stay
inside the border, for example with `anchors.fill: parent` and
`anchors.margins: 1`. It has no implicit size, so set `width` and `height`
or anchor it. Change `border.style` to `Border.Rounded`, `Border.Double`, `Border.Ascii` or
`"none"` like any Rectangle; types without a `border` group, such as
[TabBar](tabbar.md) or [TableView](tableview.md), are typically wrapped in a
Panel to get a titled frame.

### Example

```qml
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    Panel {
        anchors.centerIn: parent
        width: 44
        height: 8
        title: "Pod Metrics"
        titleBadge: "[RUNNING]"
        titleBadgeColor: "green"
        titleTrailing: "v1.2.0"
        footer: "[q] Quit"
        footerTrailing: "CPU: 42%"
        border.style: Border.Rounded

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Text { text: "Memory: 512 MiB / 2048 MiB" }
            Button { text: "Quit"; focus: true; onClicked: Qt.quit() }
        }
    }
}
```
