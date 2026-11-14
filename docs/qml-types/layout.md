---
title: Layout
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Layout
category: layout
summary: Provides attached properties for items positioned in RowLayout, ColumnLayout, or GridLayout.
---

# Layout Attached type

Provides attached properties for items positioned in RowLayout, ColumnLayout, or GridLayout. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Attached Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `Layout.minimumWidth` | `int` | `0` | Holds the minimum width in character cells that the layout will assign to this item. |
| `Layout.minimumHeight` | `int` | `0` | Holds the minimum height in rows that the layout will assign to this item. |
| `Layout.preferredWidth` | `int` | `-1` | Holds the preferred width in character cells. If `-1`, the item's `implicitWidth` is used. |
| `Layout.preferredHeight` | `int` | `-1` | Holds the preferred height in rows. If `-1`, the item's `implicitHeight` is used. |
| `Layout.maximumWidth` | `int` | `INT_MAX` (`2147483647`) | Holds the maximum width in character cells that the layout will assign to this item. |
| `Layout.maximumHeight` | `int` | `INT_MAX` (`2147483647`) | Holds the maximum height in rows that the layout will assign to this item. |
| `Layout.fillWidth` | `bool` | `false` | Whether the item should expand to fill remaining horizontal space in the layout. |
| `Layout.fillHeight` | `bool` | `false` | Whether the item should expand to fill remaining vertical space in the layout. |
| `Layout.horizontalStretch` | `int` | `1` | Holds the relative horizontal stretch factor when distributing extra width among expanding items. |
| `Layout.verticalStretch` | `int` | `1` | Holds the relative vertical stretch factor when distributing extra height among expanding items. |
| `Layout.row` | `int` | `-1` | Holds the 0-based row index for positioning this item in a [GridLayout](gridlayout.md). |
| `Layout.column` | `int` | `-1` | Holds the 0-based column index for positioning this item in a [GridLayout](gridlayout.md). |
| `Layout.rowSpan` | `int` | `1` | Holds the number of rows spanned by this item in a [GridLayout](gridlayout.md). |
| `Layout.columnSpan` | `int` | `1` | Holds the number of columns spanned by this item in a [GridLayout](gridlayout.md). |
| `Layout.alignment` | `Qt::Alignment` | `Qt.AlignLeft \| Qt.AlignTop` | Holds the alignment flags (such as `Qt.AlignHCenter` or `Qt.AlignVCenter`) used to align the item within its allocated cell. |

## Detailed Description

Layout provides attached properties used by child items of [RowLayout](rowlayout.md),
[ColumnLayout](columnlayout.md), and [GridLayout](gridlayout.md) to define how they
resize, align, and position themselves within their parent layout.

By attaching `Layout.fillWidth: true` or `Layout.fillHeight: true`, an item requests
that any excess space in the layout after sizing all items to their preferred sizes
be allocated to it. When multiple items request expansion, extra space is divided
proportionally according to `Layout.horizontalStretch` and `Layout.verticalStretch`.

In [GridLayout](gridlayout.md), `Layout.row`, `Layout.column`, `Layout.rowSpan`,
and `Layout.columnSpan` determine the exact grid cell coordinates and cell span.

### Example

<<< ../../examples/gallery/examples/Layouts.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`).
