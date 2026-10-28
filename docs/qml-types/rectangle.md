---
title: Rectangle
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Rectangle
category: core
summary: Renders a filled rectangular area on the character grid with optional borders and gradients.
---

# Rectangle QML type

Renders a filled rectangular area on the character grid with optional borders and gradients. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `color` | `"transparent"` | Holds the background fill color of the rectangle. |
| `border` | `Border` |  | Read-only. Holds the border configuration ([Border](border.md)), including `style` and `color`. |
| `gradient` | `Gradient` | `null` | Holds an optional [Gradient](gradient.md) applied across the rectangle background. |

## Detailed Description

Rectangle is the fundamental visual container in QmlTui, filling a rectangular
region of character cells with an optional background `color`, an optional
[Gradient](gradient.md), and a configurable [Border](border.md).

When `border.style` is anything other than `Border.None`, the perimeter cells are drawn
with box-drawing glyphs (`Border.Single`, `Border.Double`, `Border.Rounded`, or `Border.Ascii`). The presence
of a border automatically adds 1 character cell of internal padding along each
edge (`leftPadding`, `rightPadding`, `topPadding`, and `bottomPadding`), insetting both
un-anchored child items and child items anchored to `parent` (`anchors.fill: parent`, `anchors.centerIn: parent`, or parent edge anchors) inside the border perimeter. Setting `padding` (or side-specific `*Padding`) adds additional interior inset inside the border.

Background color defaults to `"transparent"`, which lets content beneath show through.
Set `color: "default"` to paint using the terminal's native background color, or specify
any hex or named color.

### Example

<<< ../../examples/gallery/examples/Rectangle.qml{qml}

### Gallery Example

![Rectangle recorded demo](../assets/casts/components/rectangle.gif)

See [`examples/gallery/examples/Rectangle.qml`](../../examples/gallery/examples/Rectangle.qml) (`./build/examples/gallery/qmltui_gallery --example Rectangle`, or open in the [WebAssembly Playground](../playground.md?example=Rectangle.qml)).
