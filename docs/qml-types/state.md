---
title: State
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::State
category: animation and state
summary: Defines a named visual configuration of an item.
---

# State QML type

Defines a named visual configuration of an item. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `name` | `string` | `""` | Holds the unique name identifying this state. |
| `when` | `bool` | `false` | Holds a condition expression that automatically activates this state when true. |
| `changes` | `list<QtObject>` |  | Read-only. Holds the list of changes applied when entering this state. This is the default property. |

## Detailed Description

State describes an alternative visual appearance or configuration of an item.
Within a State, one or more [PropertyChanges](propertychanges.md) objects define
the property values applied to target items while this state is active.

States can be activated explicitly by assigning the item's `state` string property,
or declaratively using the `when` condition. Transitions between states can be
animated using [Transition](transition.md).

### Example

```qml
import QtQml
import Qamui

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
        id: indicator
        anchors.centerIn: parent
        width: 24
        height: 5
        border.style: "single"

        Text {
            id: label
            anchors.centerIn: parent
            text: "Inactive (OFF)"
        }

        states: [
            State {
                name: "active"
                when: root.active
                PropertyChanges {
                    target: label
                    properties: {
                        "text": "Active (ON)"
                    }
                }
            }
        ]
    }
}
```
