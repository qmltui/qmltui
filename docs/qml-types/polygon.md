---
title: Polygon
kind: qml
import: QmlTui
since: "0.4"
inherits: CanvasShape
cpp: qmltui::Polygon
category: canvas
summary: Draws a sequence of connected straight line segments through 2D points on a Canvas.
---

# Polygon QML type

Draws a sequence of connected straight line segments through 2D points on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [CanvasShape](canvasshape.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `points` | `list<var>` |  | Holds the sequence of vertices, specified as `Qt.point(x, y)` objects or `[x, y]` coordinate pairs. |
| `closed` | `bool` | `true` | Whether to automatically connect the final vertex back to the first vertex to form a closed shape. |

## Detailed Description

Polygon draws a series of connected line segments between an ordered list of
2D points on a parent [Canvas](canvas.md). Coordinates in `points` are given in
sub-cell canvas pixels.

When `closed` is `true` (the default), an additional closing line segment is drawn
from the last point back to the first point. When `false`, the path remains open.

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

        Polygon {
            color: "yellow"
            closed: true
            points: [
                Qt.point(30, 5),
                Qt.point(50, 35),
                Qt.point(10, 35)
            ]
        }
    }
}
```

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`, or open in the [WebAssembly Playground](../playground.md?example=Canvas.qml)).
