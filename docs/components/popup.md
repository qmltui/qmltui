---
title: Popup
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Popup
category: overlays and navigation
summary: _TODO_
---

# Popup QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `modal` | `bool` |  | _TODO_ |
| `dimBackground` | `bool` |  | _TODO_ |
| `closePolicy` | `int` |  | _TODO_ |
| `initialFocusItem` | `Item` |  | _TODO_ |
| `owner` | `Item` |  | _TODO_ |
| `parentPopup` | `Popup` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `opened()` | _TODO_ |
| `closed()` | _TODO_ |
| `accepted()` | _TODO_ |
| `rejected()` | _TODO_ |
| `finished(bool accepted)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void open()` | _TODO_ |
| `void close()` | _TODO_ |
| `void accept()` | _TODO_ |
| `void reject()` | _TODO_ |

## Enumeration Popup.ClosePolicies

| Value | Description |
| :--- | :--- |
| `Popup.NoAutoClose` | _TODO_ |
| `Popup.CloseOnEscape` | _TODO_ |
| `Popup.CloseOnOutsideClick` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    // _TODO_ show Popup in use
}
```
