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
| `model` | `var` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `orientation` | `Qt::Orientation` |  | _TODO_ |
| `spacing` | `int` |  | _TODO_ |
| `currentIndex` | `int` |  | _TODO_ |
| `highlightColor` | `rgba` |  | _TODO_ |
| `highlightBackgroundColor` | `rgba` |  | _TODO_ |
| `reuseItems` | `bool` |  | _TODO_ |
| `rootIndex` | `QModelIndex` |  | _TODO_ |

## Attached Properties

| Property | Type | Description |
| :--- | :--- | :--- |
| `ListView.view` | `ListView` | Read-only. _TODO_ |
| `ListView.isCurrentItem` | `bool` | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `itemActivated(int index)` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    ListView {
        anchors.centerIn: parent
    }
}
```

## Usage guide

Properties include `model`, `delegate`, `orientation`, `spacing`, `currentIndex`,
`highlightColor`, and `highlightBackgroundColor`. `currentIndex` defaults to `-1` and can
be reset to `-1`.

Use the attached API for selection state:

```qml
ListView {
    model: applicationModel
    delegate: Text {
        required property string display
        color: ListView.isCurrentItem ? "black" : "white"
    }
}
```

See the [ListView example](../../examples/components/list_view/Main.qml).
