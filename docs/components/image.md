---
title: Image
kind: qml
import: Qtui.Image
since: "0.4"
inherits: Item
cpp: qtui::Image
category: display
summary: _TODO_
---

# Image QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Image` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `source` | `url` |  | _TODO_ |
| `fillMode` | `FillMode` |  | _TODO_ |
| `renderMode` | `RenderMode` |  | _TODO_ |
| `status` | `Status` |  | Read-only. _TODO_ |
| `sourceSize` | `size` |  | Read-only. _TODO_ |
| `smooth` | `bool` |  | _TODO_ |
| `alignment` | `Qt::Alignment` |  | _TODO_ |

## Enumeration Image.FillMode

| Value | Description |
| :--- | :--- |
| `Image.Stretch` | _TODO_ |
| `Image.PreserveAspectFit` | _TODO_ |
| `Image.PreserveAspectCrop` | _TODO_ |
| `Image.Tile` | _TODO_ |
| `Image.Pad` | _TODO_ |

## Enumeration Image.RenderMode

| Value | Description |
| :--- | :--- |
| `Image.HalfBlock` | _TODO_ |
| `Image.Braille` | _TODO_ |
| `Image.ASCII` | _TODO_ |
| `Image.Kitty` | _TODO_ |
| `Image.Sixel` | _TODO_ |
| `Image.Auto` | _TODO_ |

## Enumeration Image.Status

| Value | Description |
| :--- | :--- |
| `Image.Null` | _TODO_ |
| `Image.Ready` | _TODO_ |
| `Image.Loading` | _TODO_ |
| `Image.Error` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Image

Terminal {
    // _TODO_ show Image in use
}
```
