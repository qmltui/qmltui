---
title: Slider
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: _TODO_
---

# Slider QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `value` | `real` | `0.0` | _TODO_ |
| `from` | `real` | `0.0` | _TODO_ |
| `to` | `real` | `100.0` | _TODO_ |
| `stepSize` | `real` | `1.0` | _TODO_ |
| `textColor` | `rgba` | `"default"` | _TODO_ |
| `showValue` | `bool` | `true` | _TODO_ |
| `trackWidth` | `int` |  | Read-only. _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Slider {
        anchors.centerIn: parent
    }
}
```
