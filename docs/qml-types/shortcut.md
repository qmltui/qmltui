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
| `sequence` | `int` | `0` | Holds the key code or key sequence combination that activates the shortcut. |
| `enabled` | `bool` | `true` | Holds whether the shortcut is currently active and listens for key presses. |

## Signals

| Signal | Description |
| :--- | :--- |
| `activated()` | Emitted when the shortcut key combination is pressed. |

## Detailed Description

Shortcut provides a convenient way to register global keyboard shortcuts that
trigger actions regardless of which item currently possesses keyboard focus.

The `sequence` property accepts a Qt key code (for instance `Qt.Key_F1`,
`Qt.Key_Escape`, or a modified combination like `Qt.ControlModifier | Qt.Key_S`).
When the matching key is pressed and `enabled` is `true`, the `activated()`
signal is emitted.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Shortcut {
        sequence: Qt.Key_Q
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: Qt.Key_Space
        onActivated: message.text = "Shortcut activated!"
    }

    Text {
        id: message
        anchors.centerIn: parent
        text: "Press Space or Q"
    }
}
```

### Gallery Example

![Shortcut recorded demo](../assets/casts/components/shortcut.gif)

See [`examples/gallery/examples/Shortcut.qml`](../../examples/gallery/examples/Shortcut.qml) (`./build/examples/gallery/qmltui_gallery --example Shortcut`, or open in the [WebAssembly Playground](../playground.md?example=Shortcut.qml)).
