---
title: Row
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Row
category: layout
summary: Positions child items horizontally along a single row.
---

# Row QML type

Positions child items horizontally along a single row. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `spacing` | `int` | `0` | Holds the horizontal spacing in character cells between adjacent child items. |

## Detailed Description

Row is a positioner that arranges its child items horizontally in a single row,
from left to right in the order they are added. Like Qt Quick's Row, it manages
only the horizontal `x` coordinate of each child item, placing each item
immediately after the previous one with `spacing` cells between them. A child's
`y` position remains its own relative offset within the row.

Items with `visible: false` are ignored and take up no space. The `implicitWidth`
of the Row is the sum of the widths of all visible children plus the spacing
between them. The `implicitHeight` is the maximum height among all visible children.
If an explicit `width` or `height` is set on the Row, child items are not stretched;
for stretching and alignment behavior, use [RowLayout](rowlayout.md).

### Example

<<< ../../examples/gallery/examples/Layouts.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`).
