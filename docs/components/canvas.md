---
title: Canvas
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Canvas
category: canvas
summary: _TODO_
---

# Canvas QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `rgba` |  | _TODO_ |
| `fillColor` | `rgba` |  | _TODO_ |
| `renderMode` | `RenderMode` |  | _TODO_ |
| `cellColorMode` | `CellColorMode` |  | _TODO_ |
| `autoClear` | `bool` |  | _TODO_ |
| `pixelWidth` | `int` |  | Read-only. _TODO_ |
| `pixelHeight` | `int` |  | Read-only. _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void setPixel(int x, int y, bool value)` | _TODO_ |
| `void setPixel(int x, int y)` | _TODO_ |
| `void setPixelColor(int x, int y, Color color)` | _TODO_ |
| `void clear()` | _TODO_ |
| `void requestPaint()` | _TODO_ |

## Enumeration Canvas.RenderMode

| Value | Description |
| :--- | :--- |
| `Canvas.Braille` | _TODO_ |
| `Canvas.HalfBlock` | _TODO_ |

## Enumeration Canvas.CellColorMode

| Value | Description |
| :--- | :--- |
| `Canvas.LastDot` | _TODO_ |
| `Canvas.Average` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    Canvas {
        anchors.centerIn: parent
    }
}
```
