---
title: Column
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Column
category: layout
summary: Positions child items vertically along a single column.
---

# Column QML type

Positions child items vertically along a single column. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `spacing` | `int` | `0` | Holds the vertical spacing in rows between adjacent child items. |

## Detailed Description

Column is a positioner that arranges its child items vertically in a single
column, from top to bottom in the order they are added. Like Qt Quick's Column,
it manages only the vertical `y` coordinate of each child item, placing each item
immediately below the previous one with `spacing` rows between them. A child's
`x` position remains its own relative offset within the column.

Items with `visible: false` are ignored and take up no space. The `implicitHeight`
of the Column is the sum of the heights of all visible children plus the spacing
between them. The `implicitWidth` is the maximum width among all visible children.
If an explicit `width` or `height` is set on the Column, child items are not stretched;
for flexible stretching and layout alignment, use [ColumnLayout](columnlayout.md).

### Example

<<< ../../examples/gallery/examples/Layouts.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`, or open in the [WebAssembly Playground](../playground.md?example=Layouts.qml)).
