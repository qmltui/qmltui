---
title: Paragraph
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Paragraph
category: display
summary: _TODO_
---

# Paragraph QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` |  | _TODO_ |
| `textFormat` | `TextFormat` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `backgroundColor` | `rgba` |  | _TODO_ |
| `alignment` | `string` |  | _TODO_ |
| `wrapMode` | `bool` |  | _TODO_ |
| `scrollY` | `int` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `linkActivated(string link)` | _TODO_ |

## Enumeration Paragraph.TextFormat

| Value | Description |
| :--- | :--- |
| `Paragraph.AutoText` | _TODO_ |
| `Paragraph.PlainText` | _TODO_ |
| `Paragraph.RichText` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    Paragraph {
        anchors.centerIn: parent
    }
}
```
