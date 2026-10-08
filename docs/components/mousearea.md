---
title: MouseArea
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::MouseArea
category: input events
summary: _TODO_
---

# MouseArea QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `mouseX` | `int` |  | Read-only. _TODO_ |
| `mouseY` | `int` |  | Read-only. _TODO_ |
| `hoverEnabled` | `bool` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `pressed(MouseEvent event)` | _TODO_ |
| `released(MouseEvent event)` | _TODO_ |
| `clicked(MouseEvent event)` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    MouseArea {
        anchors.centerIn: parent
    }
}
```
