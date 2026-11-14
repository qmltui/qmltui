---
title: Gradient
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Gradient
category: core
summary: Defines a linear color gradient with multiple color stops.
---

# Gradient QML type

Defines a linear color gradient with multiple color stops. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
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

Gradient defines a smooth transition between colors across character cells or
subpixels. It can be applied to the background of a [Rectangle](rectangle.md),
to the text fill of a [Text](text.md) item, or to a [Canvas](canvas.md) and its
[CanvasShape](canvasshape.md) items.

A Gradient contains two or more [GradientStop](gradientstop.md) child items,
each specifying a normalized `position` from `0.0` to `1.0` and a `color`.
Cells spanned by the gradient interpolate RGB color components between adjacent stops.

### Example

<<< ../../examples/gallery/examples/Gradient.qml{qml}

### Gallery Example

See [`examples/gallery/examples/Gradient.qml`](../../examples/gallery/examples/Gradient.qml) (`./build/examples/gallery/qmltui_gallery --example Gradient`).
