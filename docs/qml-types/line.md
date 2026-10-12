---
title: Line
kind: qml
import: QmlTui
since: "0.4"
inherits: CanvasShape
cpp: qmltui::Line
category: canvas
summary: Draws a straight line segment between two points on a Canvas.
---

# Line QML type

Draws a straight line segment between two points on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [CanvasShape](canvasshape.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `x1` | `int` | `0` | Holds the horizontal start coordinate in canvas pixels. |
| `y1` | `int` | `0` | Holds the vertical start coordinate in canvas pixels. |
| `x2` | `int` | `0` | Holds the horizontal end coordinate in canvas pixels. |
| `y2` | `int` | `0` | Holds the vertical end coordinate in canvas pixels. |

## Detailed Description

Line draws a 1-pixel straight line segment connecting (`x1`, `y1`) and (`x2`, `y2`)
on its parent [Canvas](canvas.md) using Bresenham's line algorithm. Coordinates
are specified in sub-cell canvas pixels.

The line is painted with the shape's `color` (or the canvas's default `color`
if none is specified).

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
        width: 30
        height: 10
        renderMode: Canvas.Braille

        Line {
            x1: 0
            y1: 0
            x2: 59
            y2: 39
            color: "cyan"
        }

        Line {
            x1: 0
            y1: 39
            x2: 59
            y2: 0
            color: "magenta"
        }
    }
}
```
