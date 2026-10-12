---
title: PropertyChanges
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::PropertyChanges
category: animation and state
summary: Describes property modifications applied in a State.
---

# PropertyChanges QML type

Describes property modifications applied in a State. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `target` | `QtObject` | `null` | Holds the target object whose properties are altered in this state. |
| `properties` | `var` | `{}` | Holds a dictionary of property names and target values applied in this state. |
| `restoreEntryValues` | `bool` | `true` | Whether modified properties should revert to their entry values upon exiting this state. |

## Detailed Description

PropertyChanges is used inside a [State](state.md) to define how property values of
target items change when that state becomes active.

The `properties` object specifies key-value pairs matching property names on `target`.
When the state is exited, if `restoreEntryValues` is `true`, any modified properties
are restored to the values they had before the state was entered.

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
        id: box
        anchors.centerIn: parent
        width: 20
        height: 5
        border.style: "single"

        Text {
            id: statusText
            anchors.centerIn: parent
            text: "Standby"
        }

        states: [
            State {
                name: "running"
                when: root.active
                PropertyChanges {
                    target: statusText
                    properties: { "text": "Running" }
                }
            }
        ]
    }
}
```
