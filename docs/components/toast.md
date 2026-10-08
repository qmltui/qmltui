---
title: Toast
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Toast
category: overlays and navigation
summary: A host item that stacks transient, timed notification boxes in one of its corners.
---

# Toast QML type

A host item that stacks transient, timed notification boxes in one of its corners. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `rgba` | `"default"` | Holds the background colour of every toast box, where `"default"` is the terminal's own background. |
| `textColor` | `rgba` | `"default"` | Holds the colour of the message line, where `"default"` is the terminal's own foreground. |
| `infoColor` | `rgba` | `"default"` | Holds the border and title colour of toasts of type `"info"`, and of any unrecognised type. |
| `successColor` | `rgba` | `"default"` | Holds the border and title colour of toasts of type `"success"`. |
| `warningColor` | `rgba` | `"default"` | Holds the border and title colour of toasts of type `"warning"`. |
| `errorColor` | `rgba` | `"default"` | Holds the border and title colour of toasts of type `"error"`. |
| `toastWidth` | `int` | `28` | Holds the width in cells of each toast box, including its border. |
| `position` | `string` | `"top-right"` | Holds the corner of the item the stack grows from; a value containing `"bottom"` anchors it to the bottom edge and one containing `"left"` to the left edge. |

## Methods

| Method | Description |
| :--- | :--- |
| `void onTick()` | Internal timer slot that expires toasts every 100 ms; it is not meant to be called from QML. |
| `void showToast(string title, string message, string type, int timeoutMs)` | Appends a toast of the given `type` that disappears after `timeoutMs` milliseconds, clamped to at least 100. |
| `void showToast(string title, string message, string type)` | Appends a toast of the given `type` that disappears after 3000 ms. |
| `void showToast(string title, string message)` | Appends an `"info"` toast that disappears after 3000 ms. |

## Detailed Description

Toast is a container that owns a queue of notifications; the item itself
draws nothing until `showToast()` is called. Each toast is a 3-row box of
`toastWidth` cells with a border in the colour of its type, a bold first row
holding a status glyph and the title, and a second row holding the message.
The glyphs are ℹ for `"info"`, ✔ for `"success"`, ⚠ for `"warning"` and ✖
for `"error"`; any other type string is drawn like `"info"`. The title is
truncated to `toastWidth - 4` cells and the message to `toastWidth - 3`
cells; neither wraps.

Toasts are stacked in the order they were shown with one blank row between
boxes, so each entry takes 4 rows. They are right-aligned inside the item
unless `position` contains `"left"`, and start at the top unless it contains
`"bottom"`, in which case the stack is placed so that its last entry ends at
the bottom edge. Size the Toast item to the area the stack may use, typically
anchored to a corner of the [Terminal](terminal.md); the implicit size is 30
by 10 cells. Boxes are not clipped to the item, so more toasts than fit will
draw beyond it.

A 100 ms timer runs only while toasts are queued and removes each toast once
its own timeout has elapsed. Toast is an ordinary [Item](item.md), not a
[Popup](popup.md): it takes no focus, ignores input and is painted in normal
stacking order, so an open modal popup and its dimming are drawn above it.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Row {
        x: 2; y: 2
        spacing: 2

        Button {
            text: "Save"
            focus: true
            onClicked: notices.showToast("Saved", "Settings written.", "success")
        }

        Button {
            text: "Quit"
            onClicked: Qt.quit()
        }
    }

    Toast {
        id: notices
        anchors.top: parent.top
        anchors.right: parent.right
        width: 32
        height: 12
        successColor: "green"
        errorColor: "red"
    }

    Component.onCompleted: notices.showToast("Welcome", "Press Save to see a toast.")
}
```
