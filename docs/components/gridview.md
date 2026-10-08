---
title: GridView
kind: qml
import: Qtui
since: "0.4"
inherits: ScrollView
cpp: qtui::GridView
category: data views
summary: _TODO_
---

# GridView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [ScrollView](scrollview.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `cellWidth` | `int` |  | _TODO_ |
| `cellHeight` | `int` |  | _TODO_ |
| `rootIndex` | `QModelIndex` |  | _TODO_ |
| `reuseItems` | `bool` |  | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    GridView {
        anchors.centerIn: parent
    }
}
```
