---
title: ColumnLayout
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::ColumnLayout
category: layout
summary: Arranges items vertically with flexible sizing and alignment.
---

# ColumnLayout QML type

Arranges items vertically with flexible sizing and alignment. [More...](#detailed-description)

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

ColumnLayout arranges its child items vertically in a column, dynamically distributing
available height according to each item's [Layout](layout.md) attached properties.
Unlike [Column](column.md), which is a passive positioner, ColumnLayout actively
resizes and positions child items to fit the available vertical and horizontal space.

Child items configure their sizing behavior using `Layout.fillHeight`, `Layout.fillWidth`,
`Layout.preferredHeight`, `Layout.minimumHeight`, `Layout.maximumHeight`, and `Layout.alignment`.
When the layout has excess height, items with `Layout.fillHeight: true` expand proportionally
to their `Layout.verticalStretch` factors.

The `spacing` property sets the number of rows between
adjacent items.

### Example

<<< ../../examples/gallery/examples/Layouts.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`, or open in the [WebAssembly Playground](../playground.md?example=Layouts.qml)).
