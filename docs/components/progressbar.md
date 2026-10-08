---
title: ProgressBar
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: display
summary: _TODO_
---

# ProgressBar QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `value` | `real` | `0.0` | _TODO_ |
| `from` | `real` | `0.0` | _TODO_ |
| `to` | `real` | `1.0` | _TODO_ |
| `showBrackets` | `bool` | `false` | _TODO_ |
| `showPercentage` | `bool` | `false` | _TODO_ |
| `barWidth` | `int` |  | Read-only. _TODO_ |
| `percentageText` | `string` |  | Read-only. _TODO_ |
| `style` | `string` | `"segmented"` | _TODO_ |
| `color` | `rgba` | `"default"` | _TODO_ |
| `fillCharacter` | `string` | `""` | _TODO_ |
| `emptyCharacter` | `string` | `""` | _TODO_ |
| `gradient` | `Gradient` | `null` | _TODO_ |
| `thresholds` | `var` |  | _TODO_ |
| `hasGradient` | `bool` |  | Read-only. _TODO_ |
| `effectiveFillChar` | `string` |  | Read-only. _TODO_ |
| `effectiveEmptyChar` | `string` |  | Read-only. _TODO_ |
| `position` | `real` |  | Read-only. _TODO_ |
| `segments` | `var` |  | Read-only. _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `var getColorAt(var pos)` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    ProgressBar {
        anchors.centerIn: parent
    }
}
```
