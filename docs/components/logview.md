---
title: LogView
kind: qml
import: Qtui
since: "0.4"
inherits: TextArea
cpp: qtui::LogView
category: data views
summary: _TODO_
---

# LogView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [TextArea](textarea.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `maximumLineCount` | `int` |  | _TODO_ |
| `followTail` | `bool` |  | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void appendLine(string line)` | _TODO_ |
| `void appendText(string text)` | _TODO_ |
| `void clear()` | _TODO_ |
| `void copySelection()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    LogView {
        anchors.centerIn: parent
    }
}
```
