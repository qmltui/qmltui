---
title: BusyIndicator
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Text
cpp: qtui::Text
category: display
summary: _TODO_
---

# BusyIndicator QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Text](text.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `style` | `int` |  | _TODO_ |
| `running` | `bool` | `true` | _TODO_ |
| `interval` | `int` | `100` | _TODO_ |
| `_frameIndex` | `int` | `0` | _TODO_ |
| `_frames` | `list<string>` |  | Read-only. _TODO_ |
| `_currentSymbol` | `string` |  | Read-only. _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    BusyIndicator {
        anchors.centerIn: parent
    }
}
```
