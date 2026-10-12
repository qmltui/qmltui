---
title: Gradient
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Gradient
category: core
summary: Defines a linear color gradient with multiple color stops.
---

# Gradient QML type

Defines a linear color gradient with multiple color stops. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `stops` | `list<GradientStop>` |  | Read-only. Holds the list of [GradientStop](gradientstop.md) definitions. This is the default property. |
| `offset` | `real` | `0.0` | Holds the normalized offset shifting the gradient phase. |
| `orientation` | `Orientation` | `Gradient.Vertical` | Holds the linear direction: `Gradient.Vertical` (top-to-bottom) or `Gradient.Horizontal` (left-to-right). |

## Enumeration Gradient.Orientation

| Value | Description |
| :--- | :--- |
| `Gradient.Vertical` | Vertical gradient orientation (top to bottom). |
| `Gradient.Horizontal` | Horizontal gradient orientation (left to right). |

## Signals

| Signal | Description |
| :--- | :--- |
| `updated()` | Emitted when any property or stop within the gradient is modified. |

## Detailed Description

Gradient defines a smooth transition between colors across character cells.
It can be applied to the background of a [Rectangle](rectangle.md) or to
the text fill of a [Text](text.md) item.

A Gradient contains two or more [GradientStop](gradientstop.md) child items,
each specifying a normalized `position` from `0.0` to `1.0` and a `color`.
Cells spanned by the gradient interpolate RGB color components between adjacent stops.

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
        anchors.centerIn: parent
        width: 30
        height: 8
        border.style: "single"

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "cyan" }
            GradientStop { position: 1.0; color: "blue" }
        }

        Text {
            anchors.centerIn: parent
            text: "Gradient Fill"
        }
    }
}
```
