---
title: LogView
kind: qml
import: Qamui
since: "0.4"
inherits: TextArea
cpp: qamui::LogView
category: data views
summary: _TODO_
---

# LogView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
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
import Qamui

Terminal {
    LogView {
        anchors.centerIn: parent
    }
}
```
