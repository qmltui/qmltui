---
title: CanvasRectangle
kind: qml
import: QmlTui
since: "0.4"
inherits: CanvasShape
cpp: qmltui::CanvasRectangle
category: canvas
summary: Draws a rectangle outline or filled rectangle on a Canvas.
---

# CanvasRectangle QML type

Draws a rectangle outline or filled rectangle on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [CanvasShape](canvasshape.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `fill` | `bool` | `false` | Whether to fill the interior of the rectangle or draw only its outline. |

## Detailed Description

CanvasRectangle draws an axis-aligned rectangle on its parent [Canvas](canvas.md).
The rectangle bounds are specified by the item's `x`, `y`, `width`, and `height`
properties (measured in sub-cell canvas pixels).

If `fill` is `true`, all pixels within the rectangle are painted with the shape's
`color`. If `false`, only the perimeter 1-pixel border is drawn.

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

        CanvasRectangle {
            x: 4
            y: 4
            width: 52
            height: 32
            color: "cyan"
            fill: false
        }
    }
}
```
