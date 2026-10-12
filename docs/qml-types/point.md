---
title: Point
kind: qml
import: QmlTui
since: "0.4"
inherits: CanvasShape
cpp: qmltui::Point
category: canvas
summary: Draws a single sub-cell pixel or dot on a Canvas.
---

# Point QML type

Draws a single sub-cell pixel or dot on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [CanvasShape](canvasshape.md) |

## Detailed Description

Point draws a single dot or sub-cell pixel on its parent [Canvas](canvas.md).
The pixel location is defined by the item's `x` and `y` coordinates in canvas
sub-cell pixels. The pixel is painted using the shape's `color` (or the canvas's
default `color` if unset).

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

    Canvas {
        anchors.centerIn: parent
        width: 20
        height: 8
        renderMode: Canvas.Braille

        Point {
            x: 10
            y: 10
            color: "red"
        }

        Point {
            x: 20
            y: 15
            color: "yellow"
        }
    }
}
```
