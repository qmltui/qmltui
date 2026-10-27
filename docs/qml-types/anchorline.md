---
title: AnchorLine
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::AnchorLine
category: core
summary: Represents an edge or center alignment line used for anchoring items.
---

# AnchorLine QML type

Represents an edge or center alignment line used for anchoring items. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not directly instantiable |

## Enumeration AnchorLine.Edge

| Value | Description |
| :--- | :--- |
| `AnchorLine.Left` | The left edge alignment line of an item. |
| `AnchorLine.Right` | The right edge alignment line of an item. |
| `AnchorLine.Top` | The top edge alignment line of an item. |
| `AnchorLine.Bottom` | The bottom edge alignment line of an item. |
| `AnchorLine.HorizontalCenter` | The horizontal center line bisecting an item. |
| `AnchorLine.VerticalCenter` | The vertical center line bisecting an item. |

## Detailed Description

AnchorLine represents a specific edge or center boundary on an [Item](item.md).
It is exposed as read-only properties on Item: `left`, `right`, `top`, `bottom`,
`horizontalCenter`, and `verticalCenter`.

AnchorLine instances cannot be instantiated directly in QML; they are passed
as targets to [Anchors](anchors.md) properties (such as `anchors.left: parent.left`).

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

    Rectangle {
        id: box
        width: 20
        height: 5
        border.style: Border.Single
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 2

        Text {
            anchors.centerIn: parent
            text: "Anchored Box"
        }
    }
}
```

### Gallery Example

![Layouts recorded demo](../assets/casts/components/layouts.gif)

See [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml) (`./build/examples/gallery/qmltui_gallery --example Layouts`, or open in the [WebAssembly Playground](../playground.md?example=Layouts.qml)).
