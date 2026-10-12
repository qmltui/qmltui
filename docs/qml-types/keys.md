---
title: Keys
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Keys
category: input events
summary: Provides attached properties and signals for handling keyboard input.
---

# Keys QML type

Provides attached properties and signals for handling keyboard input. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `priority` | `Priority` | `Keys.BeforeItem` | Holds whether attached signal handlers run before (`Keys.BeforeItem`) or after (`Keys.AfterItem`) the item's own key event handling. |
| `forwardTo` | `list<Item>` |  | Read-only. Holds a list of target items to which unhandled key events are forwarded in order. |

## Signals

| Signal | Description |
| :--- | :--- |
| `pressed(KeyEvent event)` | Emitted when any key is pressed while the item has active focus. |
| `returnPressed(KeyEvent event)` | Emitted when Return or Enter is pressed. |
| `escapePressed(KeyEvent event)` | Emitted when Escape is pressed. |
| `spacePressed(KeyEvent event)` | Emitted when Spacebar is pressed. |
| `upPressed(KeyEvent event)` | Emitted when the Up arrow key is pressed. |
| `downPressed(KeyEvent event)` | Emitted when the Down arrow key is pressed. |
| `leftPressed(KeyEvent event)` | Emitted when the Left arrow key is pressed. |
| `rightPressed(KeyEvent event)` | Emitted when the Right arrow key is pressed. |
| `tabPressed(KeyEvent event)` | Emitted when the Tab key is pressed. |

## Enumeration Keys.Priority

| Value | Description |
| :--- | :--- |
| `Keys.BeforeItem` | Attached Keys signal handlers execute before the target item's built-in keyPressEvent. |
| `Keys.AfterItem` | The target item processes the key event first; attached Keys handlers execute only if the item leaves the event unaccepted. |

## Detailed Description

Keys provides attached properties and signal handlers for intercepting keyboard
input delivered to focused items or the root [Terminal](terminal.md).

For an item to receive key events via Keys, it must have `focus: true` and active focus.
Specific signals such as `onReturnPressed`, `onEscapePressed`, and `onSpacePressed`
provide convenient hooks for standard terminal navigation keys, while `onPressed`
catches all key events via a [KeyEvent](keyevent.md) object.

Key events can be forwarded to other items by adding them to the `forwardTo` list.
Once an event handler sets `event.accepted = true`, event propagation terminates.

### Example

```qml
import QtQml
import Qamui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Item {
        anchors.centerIn: parent
        width: 30
        height: 5
        focus: true

        Keys.onUpPressed: event => {
            label.text = "Up pressed"
            event.accepted = true
        }
        Keys.onDownPressed: event => {
            label.text = "Down pressed"
            event.accepted = true
        }

        Text {
            id: label
            anchors.centerIn: parent
            text: "Use Up/Down arrows"
        }
    }
}
```
