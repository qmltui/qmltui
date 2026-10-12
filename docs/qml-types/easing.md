---
title: Easing
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Easing
category: animation and state
summary: Defines easing curve types for animations.
---

# Easing QML type

Defines easing curve types for animations. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Enumeration Easing.Type

| Value | Description |
| :--- | :--- |
| `Easing.Linear` | Linear progression at uniform speed with no acceleration or deceleration. |
| `Easing.InQuad` | Acceleration from zero velocity following a quadratic curve. |
| `Easing.OutQuad` | Deceleration to zero velocity following a quadratic curve. |
| `Easing.InOutQuad` | Acceleration followed by deceleration using a quadratic curve. |
| `Easing.InCubic` | Acceleration from zero velocity following a cubic curve. |
| `Easing.OutCubic` | Deceleration to zero velocity following a cubic curve. |
| `Easing.InOutCubic` | Acceleration followed by deceleration using a cubic curve. |
| `Easing.OutBounce` | Bouncing effect that decelerates, overshoots, and rebounds to rest at the target. |

## Detailed Description

Easing provides curve types used by [PropertyAnimation](propertyanimation.md)
to control the rate of change of an animated value over time.

By assigning `easing: Easing.OutQuad` or `Easing.InOutCubic`, animations feel
more natural and responsive than standard linear transitions.

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
        id: card
        anchors.centerIn: parent
        width: 12
        height: 3
        border.style: Border.Single

        NumberAnimation {
            target: card
            property: "width"
            from: 12
            to: 36
            duration: 400
            easing: Easing.OutQuad
            running: true
        }

        Text {
            anchors.centerIn: parent
            text: "Easing Demo"
        }
    }
}
```
