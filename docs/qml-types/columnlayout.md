---
title: ColumnLayout
kind: qml
import: Qamui
since: "0.4"
inherits: LinearLayout
cpp: qamui::ColumnLayout
category: layout
summary: Arranges items vertically with flexible sizing and alignment.
---

# ColumnLayout QML type

Arranges items vertically with flexible sizing and alignment. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | `LinearLayout` |

## Detailed Description

ColumnLayout arranges its child items vertically in a column, dynamically distributing
available height according to each item's [Layout](layout.md) attached properties.
Unlike [Column](column.md), which is a passive positioner, ColumnLayout actively
resizes and positions child items to fit the available vertical and horizontal space.

Child items configure their sizing behavior using `Layout.fillHeight`, `Layout.fillWidth`,
`Layout.preferredHeight`, `Layout.minimumHeight`, `Layout.maximumHeight`, and `Layout.alignment`.
When the layout has excess height, items with `Layout.fillHeight: true` expand proportionally
to their `Layout.verticalStretch` factors.

The `spacing` property (inherited from `LinearLayout`) sets the number of rows between
adjacent items.

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            border.style: "single"
            Text {
                anchors.centerIn: parent
                text: "Header"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.style: "single"
            Text {
                anchors.centerIn: parent
                text: "Body"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            border.style: "single"
            Text {
                anchors.centerIn: parent
                text: "Footer"
            }
        }
    }
}
```
