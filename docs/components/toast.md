---
title: Toast
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Toast
category: overlays and navigation
summary: _TODO_
---

# Toast QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `rgba` |  | _TODO_ |
| `textColor` | `rgba` |  | _TODO_ |
| `infoColor` | `rgba` |  | _TODO_ |
| `successColor` | `rgba` |  | _TODO_ |
| `warningColor` | `rgba` |  | _TODO_ |
| `errorColor` | `rgba` |  | _TODO_ |
| `toastWidth` | `int` |  | _TODO_ |
| `position` | `string` |  | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void onTick()` | _TODO_ |
| `void showToast(string title, string message, string type, int timeoutMs)` | _TODO_ |
| `void showToast(string title, string message, string type)` | _TODO_ |
| `void showToast(string title, string message)` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    // _TODO_ show Toast in use
}
```
