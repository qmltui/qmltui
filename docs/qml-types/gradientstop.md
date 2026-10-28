---
title: GradientStop
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::GradientStop
category: core
summary: Defines a color stop at a normalized position within a Gradient.
---

# GradientStop QML type

Defines a color stop at a normalized position within a Gradient. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `position` | `real` | `0.0` | Holds the normalized position of the stop along the gradient axis, from `0.0` to `1.0`. |
| `color` | `color` | `"white"` | Holds the color of the gradient at this stop. |

## Signals

| Signal | Description |
| :--- | :--- |
| `updated()` | Emitted when either `position` or `color` is modified. |

## Detailed Description

GradientStop specifies the color at a particular location along the axis of a
[Gradient](gradient.md).

Multiple stops are defined within a Gradient to create smooth multi-stop color
ramps. `position` ranges from `0.0` (start of the gradient axis) to `1.0` (end of the axis).

### Example

<<< ../../examples/gallery/examples/Gradient.qml{qml}

### Gallery Example

![Rectangle recorded demo](../assets/casts/components/rectangle.gif)

See [`examples/gallery/examples/Gradient.qml`](../../examples/gallery/examples/Gradient.qml) (`./build/examples/gallery/qmltui_gallery --example Gradient`, or open in the [WebAssembly Playground](../playground.md?example=Gradient.qml)).
