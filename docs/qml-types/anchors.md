---
title: Anchors
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Anchors
category: core
summary: Grouped property defining relative positioning constraints between items.
---

# Anchors QML type

Grouped property defining relative positioning constraints between items. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not directly instantiable |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `fill` | `QtObject` | `null` | Anchors all four edges to match the target item's bounds. |
| `centerIn` | `QtObject` | `null` | Centers this item both horizontally and vertically inside the target item. |
| `left` | `AnchorLine` | `null` | Anchors the left edge of this item to the specified [AnchorLine](anchorline.md). |
| `right` | `AnchorLine` | `null` | Anchors the right edge of this item to the specified [AnchorLine](anchorline.md). |
| `top` | `AnchorLine` | `null` | Anchors the top edge of this item to the specified [AnchorLine](anchorline.md). |
| `bottom` | `AnchorLine` | `null` | Anchors the bottom edge of this item to the specified [AnchorLine](anchorline.md). |
| `horizontalCenter` | `AnchorLine` | `null` | Anchors the horizontal center of this item to the specified [AnchorLine](anchorline.md). |
| `verticalCenter` | `AnchorLine` | `null` | Anchors the vertical center of this item to the specified [AnchorLine](anchorline.md). |
| `margins` | `int` | `0` | Holds the default distance in character cells applied to all anchor edges. |
| `leftMargin` | `int` | `margins` | Holds the horizontal margin in cells for the left anchor. |
| `rightMargin` | `int` | `margins` | Holds the horizontal margin in cells for the right anchor. |
| `topMargin` | `int` | `margins` | Holds the vertical margin in rows for the top anchor. |
| `bottomMargin` | `int` | `margins` | Holds the vertical margin in rows for the bottom anchor. |

## Detailed Description

Anchors provide a declarative way to specify geometric relationships and alignments
between items on the character cell grid. Every [Item](item.md) exposes an `anchors`
grouped property.

Items can be anchored to their parent or to siblings. Anchoring both `left` and `right`
automatically resolves the item's width; anchoring both `top` and `bottom` resolves its
height. Using `anchors.fill: target` sets all four edges, while `anchors.centerIn: target`
centers the item within the target without altering its dimensions.

All margins are measured in integer character cells and rows.

### Example

<<< ../../examples/gallery/examples/Anchors.qml{qml}

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Anchors.qml`](../../examples/gallery/examples/Anchors.qml) (`./build/examples/gallery/qmltui_gallery --example Anchors`, or open in the [WebAssembly Playground](../playground.md?example=Anchors.qml)).
