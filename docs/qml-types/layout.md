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

| Property | Type | Description |
| :--- | :--- | :--- |
| `Layout.minimumWidth` | `int` | Holds the minimum width in character cells that the layout will assign to this item. |
| `Layout.minimumHeight` | `int` | Holds the minimum height in rows that the layout will assign to this item. |
| `Layout.preferredWidth` | `int` | Holds the preferred width in character cells. If `-1`, the item's `implicitWidth` is used. |
| `Layout.preferredHeight` | `int` | Holds the preferred height in rows. If `-1`, the item's `implicitHeight` is used. |
| `Layout.maximumWidth` | `int` | Holds the maximum width in character cells that the layout will assign to this item. |
| `Layout.maximumHeight` | `int` | Holds the maximum height in rows that the layout will assign to this item. |
| `Layout.fillWidth` | `bool` | Whether the item should expand to fill remaining horizontal space in the layout. |
| `Layout.fillHeight` | `bool` | Whether the item should expand to fill remaining vertical space in the layout. |
| `Layout.horizontalStretch` | `int` | Holds the relative horizontal stretch factor when distributing extra width among expanding items. |
| `Layout.verticalStretch` | `int` | Holds the relative vertical stretch factor when distributing extra height among expanding items. |
| `Layout.row` | `int` | Holds the 0-based row index for positioning this item in a [GridLayout](gridlayout.md). |
| `Layout.column` | `int` | Holds the 0-based column index for positioning this item in a [GridLayout](gridlayout.md). |
| `Layout.rowSpan` | `int` | Holds the number of rows spanned by this item in a [GridLayout](gridlayout.md). |
| `Layout.columnSpan` | `int` | Holds the number of columns spanned by this item in a [GridLayout](gridlayout.md). |
| `Layout.alignment` | `Qt::Alignment` | Holds the alignment flags (such as `Qt.AlignHCenter` or `Qt.AlignVCenter`) used to align the item within its allocated cell. |

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

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 10
            border.style: Border.Single
            Text {
                anchors.centerIn: parent
                text: "Panel A"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.horizontalStretch: 2
            border.style: Border.Double
            Text {
                anchors.centerIn: parent
                text: "Panel B (2x)"
            }
        }
    }
}
```
