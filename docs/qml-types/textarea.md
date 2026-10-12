---
title: TextArea
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::TextArea
category: inputs
summary: A multi-line editable text area item for terminal applications.
---

# TextArea QML type

A multi-line editable text area item for terminal applications. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the multi-line plain text content. |
| `color` | `rgba` | `"default"` | Holds the foreground color of the text. |
| `placeholderText` | `string` | `""` | Holds the hint text displayed when `text` is empty. |
| `cursorPosition` | `int` | `0` | Holds the linear character offset of the cursor within `text`. |
| `selectionStart` | `int` | `0` | Read-only. Holds the starting character index of the active selection. |
| `selectionEnd` | `int` | `0` | Read-only. Holds the ending character index of the active selection. |
| `selectedText` | `string` | `""` | Read-only. Holds the text currently selected. |
| `readOnly` | `bool` | `false` | Whether text editing is disabled. |
| `contentX` | `int` | `0` | Holds the horizontal scroll offset in character cells. |
| `contentY` | `int` | `0` | Holds the vertical scroll offset in rows. |

## Signals

| Signal | Description |
| :--- | :--- |
| `accepted()` | Emitted when Ctrl+Return or Enter is pressed. |

## Methods

| Method | Description |
| :--- | :--- |
| `void selectAll()` | Selects the entire text document. |
| `void clearSelection()` | Clears the active selection without modifying the text. |
| `void copy()` | Copies the selected text to the clipboard. |
| `void cut()` | Cuts the selected text to the clipboard. |
| `void paste(string text)` | Inserts text at the current cursor position. |
| `void undo()` | Reverts the last edit operation. |
| `void redo()` | Reapplies the last undone edit operation. |

## Detailed Description

TextArea provides a multi-line text editor with full cursor movement, text
selection, copy/paste, and undo/redo support in a character grid.

Pressing Enter or Return inserts a newline character (`\n`) and moves the cursor
to the start of the next row. Navigation across rows and columns is handled via
arrow keys, Page Up, Page Down, Home, and End. When text exceeds the visible
area, `contentX` and `contentY` track the scroll offsets automatically so the
cursor remains in view.

Mouse interaction allows positioning the cursor on click and selecting ranges
by dragging across rows and columns.

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

        TextArea {
            anchors.fill: parent
            anchors.margins: 1
            focus: true
            placeholderText: "Type multi-line notes..."
            text: "First line of notes\nSecond line of notes"
        }
    }
}
```
