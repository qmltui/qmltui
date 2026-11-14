---
title: RowLayout
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::RowLayout
category: layout
summary: Arranges items horizontally with flexible sizing and alignment.
---

# RowLayout QML type

Arranges items horizontally with flexible sizing and alignment. [More...](#detailed-description)

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

RowLayout arranges its child items horizontally in a row, dynamically distributing
available width according to each item's [Layout](layout.md) attached properties.
Unlike [Row](row.md), which is a passive positioner that places items at their
current sizes, RowLayout can stretch, compress, and align items across the available
horizontal and vertical space.

Child items configure their sizing behavior using `Layout.fillWidth`, `Layout.fillHeight`,
`Layout.preferredWidth`, `Layout.minimumWidth`, `Layout.maximumWidth`, and `Layout.alignment`.
When the layout has excess width, items with `Layout.fillWidth: true` expand proportionally
to their `Layout.horizontalStretch` factors.

The `spacing` property sets the number of character cells
between adjacent items.

### Example

<<< ../../examples/gallery/examples/Layouts.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`).
