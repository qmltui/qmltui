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

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text { text: "System Status: Online" }
        Text { text: "Active Tasks:  12" }
        Text { text: "Memory Usage:  48%" }
    }
}
```

### Interactive Demo & Recording

![Layouts recorded demo](../assets/casts/components/layouts.gif)

- **Live Playground**: [▶ Open `Layouts.qml` in the WebAssembly Playground](../playground.md?example=Layouts.qml)
- **Standalone CLI**: `./build/examples/gallery/qmltui_gallery --example Layouts`
- **Gallery Source**: [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml)
