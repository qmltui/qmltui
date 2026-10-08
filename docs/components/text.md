---
title: Text
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::Text
category: display
summary: _TODO_
---

# Text QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `bold` | `bool` |  | _TODO_ |
| `dim` | `bool` |  | _TODO_ |
| `italic` | `bool` |  | _TODO_ |
| `underline` | `bool` |  | _TODO_ |
| `reversed` | `bool` |  | _TODO_ |
| `horizontalAlignment` | `int` |  | _TODO_ |
| `verticalAlignment` | `int` |  | _TODO_ |
| `textFormat` | `TextFormat` |  | _TODO_ |
| `gradient` | `Gradient` |  | _TODO_ |

## Enumeration Text.TextFormat

| Value | Description |
| :--- | :--- |
| `Text.PlainText` | _TODO_ |
| `Text.RichText` | _TODO_ |
| `Text.AnsiText` | _TODO_ |
| `Text.AutoText` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    Text {
        anchors.centerIn: parent
    }
}
```
