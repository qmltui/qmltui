---
title: Circle
kind: qml
import: QmlTui
since: "0.4"
inherits: CanvasShape
cpp: qmltui::Circle
category: canvas
summary: Draws a circle outline or filled disk on a Canvas.
---

# Circle QML type

Draws a circle outline or filled disk on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [CanvasShape](canvasshape.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `cx` | `int` |  | Holds the horizontal center coordinate in canvas pixels. If not set, defaults to `x`. |
| `cy` | `int` |  | Holds the vertical center coordinate in canvas pixels. If not set, defaults to `y`. |
| `radius` | `int` | `0` | Holds the radius of the circle in canvas pixels. |
| `fill` | `bool` | `false` | Whether to fill the interior of the circle or draw only its perimeter outline. |

## Detailed Description

Circle draws a rasterized circle or filled disk on its parent [Canvas](canvas.md)
using the midpoint circle algorithm. Center coordinates (`cx`, `cy`) and `radius`
are given in sub-cell canvas pixels.

If `fill` is `true`, a solid disk is drawn using `color`. If `false`, only the
outer outline is drawn.

### Example

<<< ../../examples/gallery/examples/Canvas.qml{qml}

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`, or open in the [WebAssembly Playground](../playground.md?example=Canvas.qml)).
