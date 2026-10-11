---
title: Gradient
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Gradient
category: core
summary: _TODO_
---

# Gradient QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
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
import Qamui

Terminal {
    // _TODO_ show Gradient in use
}
```
