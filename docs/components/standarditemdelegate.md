---
title: StandardItemDelegate
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: data views
summary: _TODO_
---

# StandardItemDelegate QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | _TODO_ |
| `current` | `bool` |  | _TODO_ |
| `textColor` | `rgba` | `"default"` | _TODO_ |
| `display` | `string` |  | Read-only. _TODO_ |
| `checkable` | `bool` |  | Read-only. _TODO_ |
| `checkState` | `int` |  | Read-only. _TODO_ |
| `itemEnabled` | `bool` |  | Read-only. _TODO_ |
| `foreground` | `string` |  | Read-only. _TODO_ |
| `background` | `string` |  | Read-only. _TODO_ |
| `decoration` | `string` |  | Read-only. _TODO_ |
| `alignment` | `string` |  | Read-only. _TODO_ |
| `prefix` | `string` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `toggled(int checkState)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `var toggle()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    StandardItemDelegate {
        anchors.centerIn: parent
    }
}
```
