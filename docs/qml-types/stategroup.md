---
title: StateGroup
kind: qml
import: QmlTui
since: "0.4"
inherits:
cpp: qmltui::StateGroup
category: animation and state
summary: Provides state and transition management outside of an Item.
---

# StateGroup QML type

Provides state and transition management outside of an Item. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `state` | `string` | `""` | Holds the name of the currently active state. |
| `states` | `list<State>` |  | Read-only. Holds the list of possible states for this group. |
| `transitions` | `list<Transition>` |  | Read-only. Holds the list of transitions for this group. |

## Detailed Description

StateGroup provides a way to define states and transitions independently of an
Item. This allows state-driven visual modifications across multiple items or
within non-visual components.

StateGroup uses the same internal state controller as Item, honoring `when`
conditions, independent ownership tracking, atomic collision preflight, and
restoration behavior.

Available when built with `QMLTUI_ENABLE_PRIVATE_QT=ON`.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    id: root
    property string mode: "idle"

    Rectangle {
        id: box
        width: 20
        height: 5
        border.style: Border.Single
        border.color: "gray"

        Text {
            id: label
            anchors.centerIn: parent
            text: "Idle"
        }
    }

    StateGroup {
        id: modeGroup
        state: root.mode
        states: [
            State {
                name: "busy"
                PropertyChanges {
                    box.border.color: "yellow"
                    label.text: "Processing..."
                }
            },
            State {
                name: "done"
                PropertyChanges {
                    box.border.color: "green"
                    label.text: "Complete!"
                }
            }
        ]
        transitions: [
            Transition {
                ColorAnimation { duration: 200 }
            }
        ]
    }
}
```
