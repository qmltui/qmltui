---
title: GridLayout
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::GridLayout
category: layout
summary: Arranges items in a two-dimensional grid of rows and columns.
---

# GridLayout QML type

Arranges items in a two-dimensional grid of rows and columns. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `columns` | `int` | `-1` | Holds the number of columns in the grid. If `-1`, column count is calculated automatically. |
| `rows` | `int` | `-1` | Holds the number of rows in the grid. If `-1`, row count is calculated automatically. |
| `columnSpacing` | `int` | `0` | Holds the horizontal spacing in character cells between adjacent columns. |
| `rowSpacing` | `int` | `0` | Holds the vertical spacing in rows between adjacent grid rows. |
| `flow` | `Flow` | `GridLayout.LeftToRight` | Holds the flow direction for automatically arranging child items that lack explicit row and column coordinates. |

## Enumeration GridLayout.Flow

| Value | Description |
| :--- | :--- |
| `GridLayout.LeftToRight` | Items are arranged horizontally row by row, wrapping to the next row when columns are exhausted. |
| `GridLayout.TopToBottom` | Items are arranged vertically column by column, wrapping to the next column when rows are exhausted. |

## Detailed Description

GridLayout arranges child items into rows and columns on a character-cell grid.
Child items can specify their exact location using the [Layout](layout.md) attached
properties `Layout.row` and `Layout.column`, or they can be placed automatically
according to the `flow` direction.

Items can span multiple cells using `Layout.rowSpan` and `Layout.columnSpan`.
Columns and rows resize to fit their content items by default, and expand when
items specify `Layout.fillWidth` or `Layout.fillHeight`. Spacing between cells
is controlled by `columnSpacing` and `rowSpacing`.

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

    GridLayout {
        anchors.centerIn: parent
        columns: 2
        columnSpacing: 2
        rowSpacing: 1

        Text { text: "Host:" }
        Text { text: "localhost" }
        Text { text: "Port:" }
        Text { text: "8080" }
        Text { text: "Status:" }
        Text { text: "Connected" }
    }
}
```
