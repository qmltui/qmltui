---
title: GridLayout
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::GridLayout
category: layout
summary: _TODO_
---

# GridLayout QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `columns` | `int` |  | _TODO_ |
| `rows` | `int` |  | _TODO_ |
| `columnSpacing` | `int` |  | _TODO_ |
| `rowSpacing` | `int` |  | _TODO_ |
| `flow` | `Flow` |  | _TODO_ |

## Enumeration GridLayout.Flow

| Value | Description |
| :--- | :--- |
| `GridLayout.LeftToRight` | _TODO_ |
| `GridLayout.TopToBottom` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    GridLayout {
        anchors.centerIn: parent
    }
}
```
