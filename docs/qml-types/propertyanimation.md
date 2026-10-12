---
title: PropertyAnimation
kind: qml
import: Qamui
since: "0.4"
inherits: Animation
cpp: qamui::PropertyAnimation
category: animation and state
summary: Animates changes to a target property over time with configurable easing.
---

# PropertyAnimation QML type

Animates changes to a target property over time with configurable easing. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Animation](animation.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `target` | `QtObject` | `null` | Holds the target object whose property is being animated. |
| `property` | `string` | `""` | Holds the name of the property to animate on `target`. |
| `duration` | `int` | `250` | Holds the duration of the animation in milliseconds. |
| `from` | `var` |  | Holds the starting value for the animated property. If unset, uses the current property value. |
| `to` | `var` |  | Holds the target end value for the animated property. |
| `easing` | `Easing::Type` | `Easing.Linear` | Holds the easing curve type applied to the animation progress. |

## Detailed Description

PropertyAnimation interpolates property values of an object between `from` and `to`
over a given `duration` in milliseconds. The interpolation rate curve is controlled
by `easing` (see [Easing](easing.md)).

PropertyAnimation can be applied directly using the `PropertyAnimation on <property>`
syntax, triggered explicitly via `start()`, or configured within a [Transition](transition.md).

For animating numeric coordinates or dimensions, prefer [NumberAnimation](numberanimation.md);
for color properties, prefer [ColorAnimation](coloranimation.md).

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
        width: 15
        height: 3
        border.style: Border.Single

        PropertyAnimation {
            target: box
            property: "width"
            from: 15
            to: 35
            duration: 300
            running: true
        }

        Text {
            anchors.centerIn: parent
            text: "Expanding"
        }
    }
}
```
