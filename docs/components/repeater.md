---
title: Repeater
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::Repeater
category: layout
summary: _TODO_
---

# Repeater QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `rootIndex` | `QModelIndex` |  | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    Repeater {
        anchors.centerIn: parent
    }
}
```
