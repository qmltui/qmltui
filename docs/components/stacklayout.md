---
title: StackLayout
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: layout
summary: A container that shows exactly one of its children at a time, chosen by index, and sizes it to fill the layout.
---

# StackLayout QML type

A container that shows exactly one of its children at a time, chosen by index, and sizes it to fill the layout. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `currentIndex` | `int` | `0` | Holds the index of the visible child; an index outside the range of children hides every child. |
| `count` | `int` |  | Read-only. Holds the number of children as read from the `children` list when the binding was first evaluated. |

## Methods

| Method | Description |
| :--- | :--- |
| `var updateVisibility()` | Reapplies visibility and geometry to all children; call it after adding or removing children dynamically. |

## Detailed Description

StackLayout is written in QML on top of [Item](item.md). On completion, and
whenever `currentIndex`, `width` or `height` changes, it walks its `children`
in declaration order, sets `visible` to true on the child at `currentIndex`
and false on all others, and moves the current child to (0, 0) with the
layout's own `width` and `height`. Hidden children keep whatever geometry
they had. Because only the current child is shown and it always fills the
layout, children can be declared with no geometry of their own.

The layout has no implicit size and does not derive one from its children,
so give it `width` and `height` or anchors. `currentIndex` is not clamped: a
negative or too-large value leaves the layout empty, which can be used to
show nothing. Children added after completion, for example from a
[Repeater](repeater.md) or `createObject()`, are not laid out until the next
index or size change; call `updateVisibility()` explicitly in that case.
`count` reads `children.length`, but the `children` list is a constant
property without change notification, so it reflects the child count at the
time the binding was first evaluated.

StackLayout switches pages instantly and does not move keyboard focus. It is
the natural partner of [TabBar](tabbar.md), whose `currentIndex` can be
bound to it. Use [StackView](stackview.md) for push and pop navigation where
pages are created on demand.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Column {
        anchors.fill: parent
        spacing: 1

        TabBar {
            id: tabs
            width: parent.width
            focus: true
            tabs: ["Status", "Actions"]
        }

        StackLayout {
            width: parent.width
            height: 6
            currentIndex: tabs.currentIndex

            Panel {
                title: "Status"
                Text { x: 2; y: 1; text: "All services healthy." }
            }

            Panel {
                title: "Actions"
                Button { x: 2; y: 1; text: "Quit"; onClicked: Qt.quit() }
            }
        }
    }
}
```
