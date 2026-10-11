---
title: Repeater
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Repeater
category: layout
summary: _TODO_
---

# Repeater QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
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
import Qamui

Terminal {
    Repeater {
        anchors.centerIn: parent
    }
}
```
