---
title: GridView
kind: qml
import: Qamui
since: "0.4"
inherits: ScrollView
cpp: qamui::GridView
category: data views
summary: _TODO_
---

# GridView QML type

_TODO_ one sentence. [More...](#detailed-description)

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
| `cellWidth` | `int` |  | _TODO_ |
| `cellHeight` | `int` |  | _TODO_ |
| `rootIndex` | `QModelIndex` |  | _TODO_ |
| `reuseItems` | `bool` |  | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    GridView {
        anchors.centerIn: parent
    }
}
```
