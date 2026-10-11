---
title: MouseArea
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::MouseArea
category: input events
summary: An invisible cell rectangle that reacts to mouse clicks, drags, hover, and the wheel.
---

# MouseArea QML type

An invisible cell rectangle that reacts to mouse clicks, drags, hover, and the wheel. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `acceptedButtons` | `Qt::MouseButtons` | `Qt.LeftButton` | The buttons this area reacts to, e.g. `Qt.LeftButton \| Qt.RightButton`. A press with any other button is left unaccepted, so an area behind can take it. |
| `pressed` | `bool` | `false` | Read-only. True while an accepted button is held after pressing this area. Works on every terminal that reports clicks; unlike `containsMouse` it needs no motion reporting. The `pressed(event)` signal shares the name, as in Qt Quick: `onPressed` binds the signal, `area.pressed` reads the property. |
| `pressedButtons` | `Qt::MouseButtons` | `Qt.NoButton` | Read-only. The accepted buttons currently held. |
| `containsPress` | `bool` | `false` | Read-only. `pressed` while the pointer is still inside the area. Button-held motion is reported in the default mouse mode, so this follows a drag out of the area and back without `hoverEnabled` — the property to bind press feedback to. |
| `mouseX` | `int` | `0` | Read-only. The pointer's last reported column, relative to the area. |
| `mouseY` | `int` | `0` | Read-only. The pointer's last reported row, relative to the area. |
| `hoverEnabled` | `bool` | `false` | Whether the area tracks the pointer without a button held: it asks the terminal for any-motion reporting, updates `mouseX`/`mouseY` on movement, and drives `containsMouse`. The request takes effect only while [Terminal](terminal.md)`.mouseReporting` is true. |
| `containsMouse` | `bool` | `false` | Read-only. True while `hoverEnabled` is set and the pointer is over this area or one of its descendants; always false on a terminal that does not report motion, so treat hover as decoration, never as the only path to a feature. |

## Signals

| Signal | Description |
| :--- | :--- |
| `pressed(MouseEvent event)` | An accepted button went down inside the area. The `pressed` property is already true. |
| `released(MouseEvent event)` | The button came up. Delivered to the area that took the press even when the pointer has left it. |
| `clicked(MouseEvent event)` | The button came up inside the area. Follows `released`. |
| `entered()` | Emitted when `containsMouse` becomes true. |
| `exited()` | Emitted when `containsMouse` becomes false, including when `hoverEnabled` is switched off or the area is hidden or disabled while hovered. |
| `wheel(MouseEvent event)` | The scroll wheel moved over the area; read `wheelDeltaY` (positive is up). Connecting a handler consumes the event; set `event.accepted = false` inside it to let a [ScrollView](scrollview.md) behind scroll anyway. With no handler connected the event passes through untouched. |

## Detailed Description

MouseArea makes a region of the cell grid clickable. Like every Qamui item its
geometry is integer cells; `mouseX`/`mouseY` are cell coordinates relative to
the area. Event handlers receive a [MouseEvent](mouseevent.md) whose `button`
holds Qt values, so Qt Quick idioms carry over:

```qml
import QtQml
import Qamui

Terminal {
    id: root

    Rectangle {
        x: 2; y: 1
        width: 20; height: 3
        color: hitArea.containsPress ? "#0369a1"
             : hitArea.containsMouse ? "#0284c7" : "#1e293b"

        Text { anchors.centerIn: parent; text: "Click or right-click" }

        MouseArea {
            id: hitArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            onClicked: event => {
                if (event.button === Qt.RightButton)
                    console.log("context menu");
            }
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }
}
```

A press is delivered to the deepest area under the pointer whose
`acceptedButtons` include the pressed button; accepting it gives the area an
implicit grab, so the following motion and the release arrive here even when
the pointer leaves the area. `clicked` fires when the release lands inside —
press, drag out, and release elsewhere is a cancelled click, and
`containsPress` reflects exactly that for visual feedback. A press held when
the area becomes hidden or disabled ends immediately, because no release will
be delivered.

Terminals differ in what they report. Clicks, button-held drags, and the wheel
work in the default mouse mode everywhere mouse reporting exists; `pressed`,
`containsPress`, and `wheel` rely only on those. Hover without a button needs
any-motion reporting, which the engine requests while any visible area sets
`hoverEnabled`; where the terminal does not support it, `containsMouse` simply
stays false. Every mouse feature is inert while `Terminal.mouseReporting` is
false, and keyboard access to the same actions is required regardless — a
terminal user may have no mouse at all.

### Differences from Qt Quick

Deliberately absent: `cursorShape` (there is no pointer to shape),
`doubleClicked` and `pressAndHold` (planned; synthesize with a `Timer` for
now), `positionChanged` (bind to `mouseX`/`mouseY` changes), the `drag`
attached group (build drags from `pressed` plus `mouseX`/`mouseY`),
`propagateComposedEvents`/`preventStealing` (Qamui's propagation is the plain
unaccepted-events-bubble model), and `scrollGestureEnabled` (no gestures).
