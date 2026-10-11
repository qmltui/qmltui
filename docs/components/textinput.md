---
title: TextInput
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::TextInput
category: inputs
summary: _TODO_
---

# TextInput QML type

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
| `placeholderColor` | `rgba` |  | _TODO_ |
| `displayText` | `string` |  | Read-only. _TODO_ |
| `overwriteMode` | `bool` |  | _TODO_ |
| `autoScroll` | `bool` |  | _TODO_ |
| `inputMask` | `string` |  | _TODO_ |
| `cursorPosition` | `int` |  | _TODO_ |
| `selectionStart` | `int` |  | Read-only. _TODO_ |
| `selectionEnd` | `int` |  | Read-only. _TODO_ |
| `selectedText` | `string` |  | Read-only. _TODO_ |
| `readOnly` | `bool` |  | _TODO_ |
| `contentX` | `int` |  | _TODO_ |
| `maximumLength` | `int` |  | _TODO_ |
| `validator` | `QRegularExpression` |  | _TODO_ |
| `acceptableInput` | `bool` |  | Read-only. _TODO_ |
| `echoMode` | `EchoMode` |  | _TODO_ |
| `echoCharacter` | `string` |  | _TODO_ |
| `cursorShape` | `CursorShape` |  | _TODO_ |
| `cursorColor` | `rgba` |  | _TODO_ |
| `cursorBlinking` | `bool` |  | _TODO_ |
| `cursorBlinkRate` | `int` |  | _TODO_ |
| `cursorVisible` | `bool` |  | _TODO_ |
| `cursorDelegate` | `string` |  | _TODO_ |
| `selectionColor` | `rgba` |  | _TODO_ |
| `selectedTextColor` | `rgba` |  | _TODO_ |
| `canUndo` | `bool` |  | Read-only. _TODO_ |
| `canRedo` | `bool` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `accepted()` | _TODO_ |
| `editingFinished()` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void select(int start, int end)` | _TODO_ |
| `void selectAll()` | _TODO_ |
| `void clearSelection()` | _TODO_ |
| `void clear()` | _TODO_ |
| `void copy()` | _TODO_ |
| `void cut()` | _TODO_ |
| `void paste(string text)` | _TODO_ |
| `void undo()` | _TODO_ |
| `void redo()` | _TODO_ |

## Enumeration TextInput.EchoMode

| Value | Description |
| :--- | :--- |
| `TextInput.Normal` | _TODO_ |
| `TextInput.Password` | _TODO_ |
| `TextInput.NoEcho` | _TODO_ |

## Enumeration TextInput.CursorShape

| Value | Description |
| :--- | :--- |
| `TextInput.Block` | _TODO_ |
| `TextInput.Underline` | _TODO_ |
| `TextInput.Beam` | _TODO_ |
| `TextInput.HollowBlock` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    TextInput {
        anchors.centerIn: parent
    }
}
```
