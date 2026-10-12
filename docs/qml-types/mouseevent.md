---
title: MouseEvent
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::MouseEvent
category: input events
summary: Carries mouse pointer coordinates, button state, and wheel deltas in character cells.
---

# MouseEvent QML type

Carries mouse pointer coordinates, button state, and wheel deltas in character cells. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | `Type` |  | Read-only. Holds the event type: `MouseEvent.Press`, `MouseEvent.Release`, `MouseEvent.Move`, or `MouseEvent.Wheel`. |
| `x` | `int` |  | Read-only. Holds the X position of the mouse event in character cell columns relative to the receiving item. |
| `y` | `int` |  | Read-only. Holds the Y position of the mouse event in character cell rows relative to the receiving item. |
| `button` | `Qt::MouseButton` |  | Read-only. The button as a Qt value: compare against `Qt.LeftButton`, `Qt.MiddleButton`, `Qt.RightButton`. `Qt.NoButton` for a wheel event and for motion without a button held. |
| `nativeButton` | `int` |  | Read-only. The terminal's own button code (SGR low bits: 0 left, 1 middle, 2 right, 3 none), for forwarding to another terminal. |
| `modifiers` | `int` |  | Read-only. Holds keyboard modifier flags (such as `Qt.ShiftModifier` or `Qt.ControlModifier`) active during the event. |
| `motion` | `bool` |  | Read-only. Whether this event represents mouse pointer movement (`type == MouseEvent.Move`). |
| `wheelDeltaX` | `int` |  | Read-only. Holds the horizontal scroll wheel delta in character cells. |
| `wheelDeltaY` | `int` |  | Read-only. Holds the vertical scroll wheel delta in rows (positive for scroll up, negative for scroll down). |
| `accepted` | `bool` | `false` | Whether a handler consumed the event. An unaccepted event bubbles to the parent item; set it to `false` in a handler to pass the event on. |

## Enumeration MouseEvent.Type

| Value | Description |
| :--- | :--- |
| `MouseEvent.Press` | A mouse button was pressed down within the item's bounds. |
| `MouseEvent.Release` | A mouse button was released. |
| `MouseEvent.Move` | The mouse cursor moved within or over the item. |
| `MouseEvent.Wheel` | The mouse wheel was rotated. |

## Detailed Description

MouseEvent encapsulates terminal mouse input decoded from VT mouse reporting
sequences (such as SGR 1006 protocol). All spatial coordinates (`x`, `y`, and
wheel deltas) are expressed in character cells, matching the discrete grid of the
terminal screen.

MouseEvents are delivered to interactive items such as [MouseArea](mousearea.md),
controls, views, and text editors. When an event is handled, setting `accepted = true`
stops further propagation to items beneath or behind in the stacking order.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 30
        height: 6
        border.style: Border.Single

        MouseArea {
            anchors.fill: parent
            onClicked: mouse => {
                info.text = "Clicked at (" + mouse.x + ", " + mouse.y + ")"
            }
        }

        Text {
            id: info
            anchors.centerIn: parent
            text: "Click inside box"
        }
    }
}
```
