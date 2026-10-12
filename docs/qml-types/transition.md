---
title: Transition
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Transition
category: animation and state
summary: Defines animated transitions between states.
---

# Transition QML type

Defines animated transitions between states. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `from` | `string` | `"*"` | Holds the origin state name that triggers this transition (`"*"` matches any state). |
| `to` | `string` | `"*"` | Holds the target destination state name that triggers this transition (`"*"` matches any state). |
| `animations` | `list<Animation>` |  | Read-only. Holds the animations played when moving between the states. This is the default property. |

## Detailed Description

Transition specifies the animations that play when an item changes its visual
[State](state.md).

By default, transitions apply in both directions if `from` and `to` are `*`,
or they can be customized for specific state changes (e.g. `from: "off"; to: "on"`).
Animations placed inside the Transition are automatically configured to animate
the properties modified by the state change.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    id: root
    property bool active: false

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Space) {
            root.active = !root.active
            event.accepted = true
        }
    }

    Rectangle {
        id: box
        anchors.centerIn: parent
        width: 20
        height: 5
        color: "blue"
        border.style: Border.Single

        states: [
            State {
                name: "active"
                when: root.active
                PropertyChanges {
                    target: box
                    properties: { "color": "green" }
                }
            }
        ]

        transitions: [
            Transition {
                ColorAnimation { duration: 300 }
            }
        ]

        Text {
            anchors.centerIn: parent
            text: "Toggle Space"
        }
    }
}
```
