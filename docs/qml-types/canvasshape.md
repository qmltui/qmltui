---
title: CanvasShape
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::CanvasShape
category: canvas
summary: Abstract base class for 2D geometric shapes drawn on a Canvas.
---

# CanvasShape QML type

Abstract base class for 2D geometric shapes drawn on a Canvas. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |
| **Instantiation** | Not directly instantiable |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `color` |  | Holds the color used to draw the shape. If not set, the parent Canvas color or gradient is used. |
| `gradient` | `Gradient` | `null` | Holds an optional [Gradient](gradient.md) interpolated across the shape's subpixel bounding box. |
| `dim` | `bool` | `false` | Holds whether cells drawn by this shape use the terminal dim/faint (`SGR 2`) attribute, overriding the parent Canvas `dim` setting when explicitly set. |

## Detailed Description

CanvasShape is the uncreatable abstract base class for declarative shapes drawn
inside a [Canvas](canvas.md). Subclasses include [Line](line.md), [Circle](circle.md),
[Point](point.md), [Polygon](polygon.md), and [CanvasRectangle](canvasrectangle.md).

Shapes are placed as child items of a Canvas. During the canvas's paint pass,
each child shape's geometry is rasterized into the canvas's pixel buffer using
the shape's individual `gradient` or `color` (or the canvas's default `gradient`
or `color` if unset).
Coordinates are measured in sub-cell canvas pixels.

### Example

<<< ../../examples/gallery/examples/Canvas.qml{qml}

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`).
