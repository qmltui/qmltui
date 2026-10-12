---
title: TextArea
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::TextArea
category: inputs
summary: A multi-line editable text area item for terminal applications.
---

# TextArea QML type

A multi-line editable text area item for terminal applications. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
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
| `cursorShape` | `CursorShape` | `TextArea.Block` | Holds the visual shape of the cursor: `Block`, `Underline`, `Beam`, or `HollowBlock`. |
| `cursorColor` | `rgba` |  | Holds the color of the text cursor. When unset or `"default"`, reverse video is used. |
| `cursorVisible` | `bool` | `true` | Whether the cursor is visible when the item has focus. |
| `textRevision` | `quint64` | `0` | Read-only. Incremented on each text change. Validates highlight layer submissions. |
| `tabChangesFocus` | `bool` | `false` | Whether Tab changes focus or is accepted as text input. When `false`, Tab inserts spaces to the next tab stop. |
| `tabStopDistance` | `int` | `4` | Number of character columns per tab stop. Tab inserts spaces aligning to multiples of this distance. |

## Signals

| Signal | Description |
| :--- | :--- |
| `accepted()` | Emitted when Ctrl+Return or Ctrl+Enter is pressed. |
| `highlightsChanged()` | Emitted when active highlight layers are modified or cleared. |

## Enumeration TextArea.CursorShape

| Value | Description |
| :--- | :--- |
| `TextArea.Block` | Renders a solid block cursor (reverse video by default, or with `cursorColor`). |
| `TextArea.Underline` | Renders an underline (`_`) in blank cells, or underlines existing characters (falls back to reverse video if already underlined). |
| `TextArea.Beam` | Renders a vertical line cursor (`│`) in blank cells, or reverse video over existing characters. |
| `TextArea.HollowBlock` | Renders a hollow rectangular block outline (`▯`) in blank cells, or underlines existing characters. |

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
| `bool setHighlightLayer(string name, list<highlightRange> ranges, quint64 revision, int priority = 0)` | Sets or updates a named highlight layer. Rejects stale revisions, empty names, or invalid bounds. Returns `true` on success. |
| `void clearHighlightLayer(string name)` | Removes the named highlight layer and schedules a repaint. |
| `void clearHighlights()` | Clears all highlight layers on the editor. |

## Detailed Description

TextArea provides a multi-line text editor with full cursor movement, text
selection, copy/paste, and undo/redo support in a character grid.

Pressing Enter or Return inserts a newline character (`\n`) and moves the cursor
to the start of the next row. Pressing Ctrl+Return or Ctrl+Enter emits the
`accepted()` signal without inserting a newline. Navigation across rows and
columns is handled via arrow keys, Page Up, Page Down, Home, and End. When text
exceeds the visible area, `contentX` and `contentY` track the scroll offsets
automatically so the cursor remains in view.

Mouse interaction allows positioning the cursor on click and selecting ranges
by dragging across rows and columns.

### Syntax and Semantic Highlighting

Formatting is applied using named highlight layers with `setHighlightLayer()`.
Every text modification increments `textRevision` and clears active highlight
layers to prevent stale styling on modified text. To maintain highlighting across
edits, re-apply layers in a `textChanged` signal handler matching the current
`textRevision`:

```qml
editor.setHighlightLayer("syntax", [
    { start: 0, length: 5, color: "green", bold: true },
    { start: 10, length: 4, color: "cyan" }
], editor.textRevision)
```

`setHighlightLayer()` returns `false` and rejects submissions if the supplied
`revision` does not match `editor.textRevision`, ensuring asynchronous or debounced
highlighters never apply misaligned ranges computed against an older revision.

Layers stack by `priority` (higher priority values take precedence over lower ones).
When priorities are equal, layers are resolved in alphabetical order of layer name.
Styles cascade attribute by attribute: a higher-priority search layer setting only
a background color preserves the foreground color and bold attributes applied by
an underlying syntax layer. Nonempty ranges automatically snap outward to grapheme
cluster boundaries to avoid partially styled multi-byte or combining sequences.

### Example

```qml
import QtQml
import QmlTui

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
