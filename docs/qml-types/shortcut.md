---
title: Shortcut
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Shortcut
category: input events
summary: Defines application-wide keyboard shortcuts that trigger actions.
---

# Shortcut QML type

Defines application-wide keyboard shortcuts that trigger actions. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `sequence` | `var` | `0` | Holds a portable key-chord string (such as `"Ctrl+S"`, `"Ctrl+Shift+P"`, `"Alt+Return"`, `"F5"`) or an integer Qt key/modifier bitmask (`Qt.ControlModifier \| Qt.Key_S`). |
| `enabled` | `bool` | `true` | Holds whether the shortcut is currently active and listens for key presses. |

## Signals

| Signal | Description |
| :--- | :--- |
| `activated()` | Emitted when the shortcut key combination is pressed. |

## Detailed Description

Shortcut registers keyboard shortcuts that fire before focused-item key dispatch
whenever the shortcut's nearest ancestor [Item](item.md) is attached to a
[Terminal](terminal.md) and is effectively visible and enabled.

The `sequence` property accepts either a single-stroke key-chord string (for
example `"Ctrl+Q"`, `"Ctrl+Shift+S"`, `"Alt+Return"`, `"Meta+P"` / `"Super+P"` /
`"Win+P"`, `"Escape"`, `"Backtab"`, or `"F1"` through `"F12"`) or a Qt key bitmask
(`Qt.Key_F1`, `Qt.Key_Escape`, or `Qt.ControlModifier | Qt.Key_S`). Note that
`Meta` / `Super` / `Win` modifiers require terminal support such as the Kitty
keyboard protocol. Only single-stroke chords are supported; multi-chord sequences
(such as `"Ctrl+X, Ctrl+S"`) or malformed strings emit a QML warning and leave
the shortcut inactive.

### Routing and Suppression Rules

- **Text input and paste**: Unmodified printable shortcuts (such as `"Q"` or `"/"`) are suppressed while an editable [TextInput](textinput.md), [TextArea](textarea.md), or custom item with `consumesPrintableKeys()` (such as [`PtyView`](https://github.com/qmltui/ptyview)) holds focus, and all shortcuts are suppressed during bracketed paste.
- **Modal popups**: While a modal [Popup](popup.md) is open, only shortcuts belonging to descendants of that popup can activate.
- **Consumption**: When a matching enabled shortcut fires, `activated()` is emitted and the key event is consumed before reaching focused items.

### Example

<<< ../../examples/gallery/examples/Shortcut.qml{qml}

### Gallery Example

![Shortcut recorded demo](../assets/casts/components/shortcut.gif)

See [`examples/gallery/examples/Shortcut.qml`](../../examples/gallery/examples/Shortcut.qml) (`./build/examples/gallery/qmltui_gallery --example Shortcut`).
