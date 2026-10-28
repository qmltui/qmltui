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

<<< ../../examples/gallery/examples/Canvas.qml{qml}

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`, or open in the [WebAssembly Playground](../playground.md?example=Canvas.qml)).
