---
title: NumberAnimation
kind: qml
import: QmlTui
since: "0.4"
inherits: PropertyAnimation
cpp: qmltui::NumberAnimation
category: animation and state
summary: Animates numeric properties over time.
---

# NumberAnimation QML type

Animates numeric properties over time. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [PropertyAnimation](propertyanimation.md) |

## Detailed Description

NumberAnimation is a specialized [PropertyAnimation](propertyanimation.md) optimized
for interpolating numeric properties such as `x`, `y`, `width`, `height`, and `opacity`.

It inherits all properties and methods from PropertyAnimation, including `duration`,
`from`, `to`, `easing`, and `loops`.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        id: box
        anchors.centerIn: parent
        width: 10
        height: 3
        border.style: Border.Single

        NumberAnimation {
            target: box
            property: "width"
            from: 10
            to: 30
            duration: 400
            running: true
        }

        Text {
            anchors.centerIn: parent
            text: "NumberAnimation"
        }
    }
}
```
