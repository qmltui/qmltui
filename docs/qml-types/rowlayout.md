---
title: RowLayout
kind: qml
import: QmlTui
since: "0.4"
inherits: LinearLayout
cpp: qmltui::RowLayout
category: layout
summary: Arranges items horizontally with flexible sizing and alignment.
---

# RowLayout QML type

Arranges items horizontally with flexible sizing and alignment. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | `LinearLayout` |

## Detailed Description

RowLayout arranges its child items horizontally in a row, dynamically distributing
available width according to each item's [Layout](layout.md) attached properties.
Unlike [Row](row.md), which is a passive positioner that places items at their
current sizes, RowLayout can stretch, compress, and align items across the available
horizontal and vertical space.

Child items configure their sizing behavior using `Layout.fillWidth`, `Layout.fillHeight`,
`Layout.preferredWidth`, `Layout.minimumWidth`, `Layout.maximumWidth`, and `Layout.alignment`.
When the layout has excess width, items with `Layout.fillWidth: true` expand proportionally
to their `Layout.horizontalStretch` factors.

The `spacing` property (inherited from `LinearLayout`) sets the number of character cells
between adjacent items.

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
        anchors.margins: 1
        spacing: 2

        Rectangle {
            Layout.preferredWidth: 20
            Layout.fillHeight: true
            border.style: Border.Single
            Text {
                anchors.centerIn: parent
                text: "Sidebar"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.style: Border.Double
            Text {
                anchors.centerIn: parent
                text: "Main Content"
            }
        }
    }
}
```

### Interactive Demo & Recording

![Layouts recorded demo](../assets/casts/components/layouts.gif)

- **Live Playground**: [▶ Open `Layouts.qml` in the WebAssembly Playground](../playground.md?example=Layouts.qml)
- **Standalone CLI**: `./build/examples/gallery/qmltui_gallery --example Layouts`
- **Gallery Source**: [`examples/gallery/examples/Layouts.qml`](../../examples/gallery/examples/Layouts.qml)
