---
title: Gradient
kind: qml
import: Qtui
since: "0.4"
inherits: 
cpp: qtui::Gradient
category: core
summary: _TODO_
---

# Gradient QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `stops` | `list<GradientStop>` |  | Read-only. _TODO_ |
| `offset` | `real` |  | _TODO_ |
| `orientation` | `Orientation` | `Gradient.Vertical` | _TODO_ |

## Enumeration Gradient.Orientation

| Value | Description |
| :--- | :--- |
| `Gradient.Vertical` | Vertical gradient orientation (top to bottom). |
| `Gradient.Horizontal` | Horizontal gradient orientation (left to right). |

## Signals

| Signal | Description |
| :--- | :--- |
| `updated()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    // _TODO_ show Gradient in use
}
```
