---
title: ColorAnimation
kind: qml
import: Qamui
since: "0.4"
inherits: PropertyAnimation
cpp: qamui::ColorAnimation
category: animation and state
summary: Animates color properties over time by smoothly interpolating color channels.
---

# ColorAnimation QML type

Animates color properties over time by smoothly interpolating color channels. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [PropertyAnimation](propertyanimation.md) |

## Detailed Description

ColorAnimation is a specialized [PropertyAnimation](propertyanimation.md) that
smoothly transitions between color values by interpolating their RGBA channels.
It is commonly used on properties like `color`, `textColor`, and `borderColor`.

Like all PropertyAnimations, it supports `duration`, `from`, `to`, and `easing`.

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

    Rectangle {
        id: box
        anchors.centerIn: parent
        width: 20
        height: 5
        border.style: "double"

        ColorAnimation {
            target: box
            property: "color"
            from: "blue"
            to: "red"
            duration: 500
            running: true
        }

        Text {
            anchors.centerIn: parent
            text: "ColorAnimation"
        }
    }
}
```
