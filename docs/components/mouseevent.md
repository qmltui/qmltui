---
title: MouseEvent
kind: qml
import: Qtui
since: "0.4"
inherits: 
cpp: qtui::MouseEvent
category: input events
summary: _TODO_
---

# MouseEvent QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | `Type` |  | Read-only. _TODO_ |
| `x` | `int` |  | Read-only. _TODO_ |
| `y` | `int` |  | Read-only. _TODO_ |
| `button` | `Qt::MouseButton` |  | Read-only. The button as a Qt value: compare against `Qt.LeftButton`, `Qt.MiddleButton`, `Qt.RightButton`. `Qt.NoButton` for a wheel event and for motion without a button held. |
| `nativeButton` | `int` |  | Read-only. The terminal's own button code (SGR low bits: 0 left, 1 middle, 2 right, 3 none), for forwarding to another terminal. |
| `modifiers` | `int` |  | Read-only. _TODO_ |
| `motion` | `bool` |  | Read-only. _TODO_ |
| `wheelDeltaX` | `int` |  | Read-only. _TODO_ |
| `wheelDeltaY` | `int` |  | Read-only. _TODO_ |
| `accepted` | `bool` | `false` | Whether a handler consumed the event. An unaccepted event bubbles to the parent item; set it to `false` in a handler to pass the event on. |

## Enumeration MouseEvent.Type

| Value | Description |
| :--- | :--- |
| `MouseEvent.Press` | _TODO_ |
| `MouseEvent.Release` | _TODO_ |
| `MouseEvent.Move` | _TODO_ |
| `MouseEvent.Wheel` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    // _TODO_ show MouseEvent in use
}
```
