---
title: TextArea
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::TextArea
category: inputs
summary: _TODO_
---

# TextArea QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `placeholderText` | `string` |  | _TODO_ |
| `cursorPosition` | `int` |  | _TODO_ |
| `selectionStart` | `int` |  | Read-only. _TODO_ |
| `selectionEnd` | `int` |  | Read-only. _TODO_ |
| `selectedText` | `string` |  | Read-only. _TODO_ |
| `readOnly` | `bool` |  | _TODO_ |
| `contentX` | `int` |  | _TODO_ |
| `contentY` | `int` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `accepted()` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void selectAll()` | _TODO_ |
| `void clearSelection()` | _TODO_ |
| `void copy()` | _TODO_ |
| `void cut()` | _TODO_ |
| `void paste(string text)` | _TODO_ |
| `void undo()` | _TODO_ |
| `void redo()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    TextArea {
        anchors.centerIn: parent
    }
}
```
