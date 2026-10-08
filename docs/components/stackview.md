---
title: StackView
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: overlays and navigation
summary: _TODO_
---

# StackView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `currentItem` | `alias` |  | Read-only. _TODO_ |
| `depth` | `int` |  | Read-only. _TODO_ |
| `stackItems` | `var` |  | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `var push(var itemOrUrl, var properties)` | _TODO_ |
| `var pop()` | _TODO_ |
| `var clear()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    StackView {
        anchors.centerIn: parent
    }
}
```
