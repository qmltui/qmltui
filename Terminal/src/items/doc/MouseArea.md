# MouseArea

## 1. Class Overview

`MouseArea` is an invisible item that captures terminal mouse input (presses, releases, clicks, and movement coordinates `mouseX`, `mouseY`).

Developers use `MouseArea` to handle mouse clicks and hover tracking in QML layouts.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/items/MouseArea.h`
- **Implementation**: `Tui/src/items/MouseArea.cpp`
- **Dependencies**: `Item.h`, `MouseEvent.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits spatial bounds, positioning, and input handling hooks.

## 4. Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `mouseX` | `int` | `mouseX` | — | `mouseXChanged` | Current X mouse position relative to this MouseArea. |
| `mouseY` | `int` | `mouseY` | — | `mouseYChanged` | Current Y mouse position relative to this MouseArea. |

## 7. Signals

#### void pressed(MouseEvent *event)
Emitted when a mouse button is pressed inside the MouseArea.

#### void released(MouseEvent *event)
Emitted when a mouse button is released over the MouseArea.

#### void clicked(MouseEvent *event)
Emitted when a mouse press and release sequence completes within the MouseArea.

## 13. QML Exposure

Registered as `MouseArea` in the `Tui` module via `QML_ELEMENT`.

## 16. Usage Example

```qml
import QtQml 2.15
import Terminal 1.0

MouseArea {
    width: 20
    height: 3
    onClicked: (event) => {
        console.log("Mouse clicked at:", event.x, event.y)
    }
}
```
