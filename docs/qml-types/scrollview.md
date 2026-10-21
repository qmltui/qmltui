---
title: ScrollView
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::ScrollView
category: data views
summary: Container providing vertical scrolling and scrollbar display for overflowing content.
---

# ScrollView QML type

Container providing vertical scrolling and scrollbar display for overflowing content. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `contentHeight` | `int` | `0` | Holds the total height in rows of the scrollable content. |
| `contentY` | `int` | `0` | Holds the vertical scroll offset in rows. |
| `scrollBarVisible` | `bool` | `true` | Whether the vertical scrollbar track and thumb are drawn. |

## Detailed Description

ScrollView provides a clipped viewport with a vertical scrollbar for content
that exceeds the visible height. It handles mouse wheel scrolling, keyboard
navigation (Up, Down, PageUp, PageDown), and mouse dragging on the scrollbar.

When `scrollBarVisible` is `true`, an overlay scrollbar is rendered along the
right edge of the viewport. Child items are clipped to the viewport boundaries.
ListView and GridView inherit from ScrollView, reusing its scrolling and scrollbar
machinery.

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

    ScrollView {
        anchors.centerIn: parent
        width: 30
        height: 6
        contentHeight: 20

        Column {
            width: parent.width
            spacing: 1

            Repeater {
                model: 15
                Text {
                    required property int index
                    text: "Item row " + index
                }
            }
        }
    }
}
```

### Gallery Example

![ScrollView recorded demo](../assets/casts/components/scroll_view.gif)

See [`examples/gallery/examples/ScrollView.qml`](../../examples/gallery/examples/ScrollView.qml) (`./build/examples/gallery/qmltui_gallery --example ScrollView`, or open in the [WebAssembly Playground](../playground.md?example=ScrollView.qml)).
