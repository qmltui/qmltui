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
| `hoverEnabled` | `bool` | `false` | Whether the area tracks the pointer without a button held: it asks the terminal for any-motion reporting, updates `mouseX`/`mouseY` on movement, and drives `containsMouse`. The request takes effect only while [Terminal](terminal.md)`.mouseReporting` is true. |
| `containsMouse` | `bool` | `false` | Read-only. True while `hoverEnabled` is set and the pointer is over this area or one of its descendants; always false on a terminal that does not report motion. |

## Signals

| Signal | Description |
| :--- | :--- |
| `pressed(MouseEvent event)` | _TODO_ |
| `released(MouseEvent event)` | _TODO_ |
| `clicked(MouseEvent event)` | _TODO_ |
| `entered()` | Emitted when `containsMouse` becomes true. |
| `exited()` | Emitted when `containsMouse` becomes false, including when `hoverEnabled` is switched off or the area is hidden or disabled while hovered. |

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
