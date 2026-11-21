---
title: Keys
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Keys
category: input events
summary: Provides attached properties and signals for handling keyboard input.
---

# Keys QML type

Provides attached properties and signals for handling keyboard input. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
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
| `pressed(KeyEvent event)` | Emitted when any key is pressed (or auto-repeated) while the item has active focus. |
| `released(KeyEvent event)` | Emitted when a key is released on terminals supporting Kitty keyboard event-type reporting (`CSI > 3 u`). |
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

For an item to receive key events via Keys, it must accept focus (`focusPolicy` set to `Qt.StrongFocus`, `Qt.TabFocus`, or `Qt.ClickFocus`—interactive controls set `Qt.StrongFocus` automatically, while plain [Item](item.md) and [Rectangle](rectangle.md) default to `Qt.NoFocus`), set `focus: true`, and hold active focus.
Specific signals such as `onReturnPressed`, `onEscapePressed`, and `onSpacePressed`
provide convenient hooks for standard terminal navigation keys, while `onPressed`
and `onReleased` catch key presses, repeats (`event.isAutoRepeat`), and releases
via a [KeyEvent](keyevent.md) object (`onReleased` and `isAutoRepeat` require a
terminal that supports the Kitty keyboard protocol).

Key events can be forwarded to other items by adding them to the `forwardTo` list.
All Keys signal handlers (including specific-key signals such as `onReturnPressed` and `onEscapePressed`) start with `event.accepted = false`; set `event.accepted = true` explicitly to terminate propagation up the parent chain.

### Example

<<< ../../examples/gallery/examples/Keys.qml{qml}

### Gallery Example

See [`examples/gallery/examples/Keys.qml`](../../examples/gallery/examples/Keys.qml) (`./build/examples/gallery/qmltui_gallery --example Keys`).
