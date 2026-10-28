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

<<< ../../examples/gallery/examples/Canvas.qml{qml}

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`, or open in the [WebAssembly Playground](../playground.md?example=Canvas.qml)).
