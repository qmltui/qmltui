---
title: Scrollback
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Scrollback
category: core
summary: _TODO_
---

# Scrollback QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `spacing` | `int` |  | _TODO_ |
| `separator` | `Component` |  | _TODO_ |
| `reuseItems` | `bool` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `outputRejected(string reason)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void append(var data)` | _TODO_ |
| `void append(Item item)` | _TODO_ |
| `bool print(string text)` | _TODO_ |
| `void clear()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    // _TODO_ show Scrollback in use
}
```
