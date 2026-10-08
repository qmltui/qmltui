---
title: Button
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: _TODO_
---

# Button QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | _TODO_ |
| `pressed` | `bool` | `false` | _TODO_ |
| `textColor` | `rgba` | `"default"` | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `clicked()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Button {
        anchors.centerIn: parent
    }
}
```
