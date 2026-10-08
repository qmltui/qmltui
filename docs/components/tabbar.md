---
title: TabBar
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::TabBar
category: overlays and navigation
summary: A one-row strip of named tabs selected with the Left and Right keys or the mouse.
---

# TabBar QML type

A one-row strip of named tabs selected with the Left and Right keys or the mouse. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `selectionColor` | `rgba` |  | Holds the background colour of the selected tab; when both it and `selectedTextColor` are unset the selected tab is drawn in reverse video instead. |
| `selectedTextColor` | `rgba` |  | Holds the text colour of the selected tab; it falls back to `textColor` when unset. |
| `color` | `rgba` | `"default"` | Holds the background colour of unselected tabs and separators, where `"default"` is the terminal's own background. |
| `textColor` | `rgba` | `"default"` | Holds the text colour of unselected tabs and separators, where `"default"` is the terminal's own foreground. |
| `tabs` | `var` |  | Holds the tab labels as a JavaScript array of strings or a string list; other array elements are converted with their string form. |
| `currentIndex` | `int` | `0` | Holds the index of the selected tab, clamped to the range of `tabs`; a value set before `tabs` is assigned is kept and clamped once the tabs arrive, and a list that shrinks clamps it as well. |

## Detailed Description

TabBar draws its `tabs` left to right on a single row. The selected tab is
rendered bold as `[ Name ]` and every other tab as `  Name  `, with a `│`
separator between neighbours. When neither `selectionColor` nor
`selectedTextColor` is set the selected tab uses the terminal's reverse-video
attribute, so it stays legible under any theme; setting either colour turns
reverse video off and uses the given colours, with the unset one falling back
to `color` or `textColor`. Tabs that do not fit within `width` are not drawn,
and the whole bar is dimmed while the item is disabled.

TabBar is focusable. While it has focus, Left and Right move `currentIndex`
by one and accept the key; the index is clamped at both ends and does not
wrap. A mouse press gives the bar focus and selects the tab under the pointer,
using the same 4-cell padding plus 1-cell separator layout as the painter.
`currentIndexChanged()` is the only notification of a selection; there is no
separate activation signal.

The implicit size is 30 by 1 cells; set `width` so all tabs fit. TabBar has
no border; wrap it in a [Rectangle](rectangle.md) or [Panel](panel.md) when
one is needed. Bind `currentIndex` to a [StackLayout](stacklayout.md) to
switch pages with the tabs.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Column {
        anchors.fill: parent
        spacing: 1

        TabBar {
            id: tabs
            width: parent.width
            focus: true
            tabs: ["Overview", "Logs", "Settings"]
            selectionColor: "blue"
            selectedTextColor: "white"
        }

        StackLayout {
            width: parent.width
            height: 5
            currentIndex: tabs.currentIndex

            Text { text: "Everything is running." }
            Text { text: "No new log lines." }
            Button { text: "Quit"; onClicked: Qt.quit() }
        }
    }
}
```
