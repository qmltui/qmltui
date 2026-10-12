---
title: ListView
kind: qml
import: Qamui
since: "0.4"
inherits: ScrollView
cpp: qamui::ListView
category: data views
summary: Displays a scrollable sequence of model delegates.
---

# ListView QML type

Displays a scrollable sequence of model delegates. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [ScrollView](scrollview.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | Holds the data model providing list items: integer count, string/object array, `ListModel`, or `QAbstractItemModel`. |
| `delegate` | `Component` |  | Holds the component template instantiated for visible items. This is the default property. |
| `orientation` | `Qt::Orientation` | `Qt.Vertical` | Holds the layout orientation of items: `Qt.Vertical` or `Qt.Horizontal`. |
| `spacing` | `int` | `0` | Holds the spacing in character cells between adjacent items. |
| `currentIndex` | `int` | `-1` | Holds the index of the currently selected or focused item, or `-1`. |
| `highlightColor` | `rgba` |  | Holds the text color override for the currently selected item. |
| `highlightBackgroundColor` | `rgba` |  | Holds the background color override for the currently selected item. |
| `reuseItems` | `bool` | `true` | Whether delegates that scroll out of view are recycled and rebound to items scrolling into view. |
| `rootIndex` | `QModelIndex` |  | Holds the parent index whose direct children the view displays when using a hierarchical model. |

## Attached Properties

| Property | Type | Description |
| :--- | :--- | :--- |
| `ListView.view` | `ListView` | Read-only. Holds the parent ListView containing this delegate instance. |
| `ListView.isCurrentItem` | `bool` | Read-only. True if this delegate instance represents the currently selected item (`currentIndex`). |

## Signals

| Signal | Description |
| :--- | :--- |
| `itemActivated(int index)` | Emitted when an item is activated with Enter or a mouse click. |

## Detailed Description

ListView provides a virtualized, scrollable list of items instantiated from a `delegate`
based on a `model`. Only delegates visible in the current viewport (plus a small overscan)
are instantiated. When `reuseItems` is `true`, delegate items are recycled as the user scrolls,
providing smooth scrolling over large models.

Keyboard navigation (Up/Down arrow keys, PageUp/PageDown, Home, End) moves `currentIndex`
and automatically scrolls the view to keep the active item visible. Mouse clicks update
`currentIndex` and emit `itemActivated()`.

Delegates access the `ListView.isCurrentItem` attached property to adjust styling
when selected.

### Example

```qml
import QtQml
import Qamui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 30
        height: 8
        border.style: Border.Single

        ListView {
            anchors.fill: parent
            anchors.margins: 1
            focus: true
            model: ["Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta"]
            currentIndex: 0

            delegate: Text {
                required property string modelData
                required property int index
                text: (ListView.isCurrentItem ? "> " : "  ") + modelData
                color: ListView.isCurrentItem ? "cyan" : "white"
                bold: ListView.isCurrentItem
            }
        }
    }
}
```
