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
| `source` | `url` |  | _TODO_ Also `image://<provider>/<id>`, served by a `qtui::ImageProvider` installed on the QML engine; changing the URL asks the provider again. Cleared when `image` is assigned. |
| `image` | `QImage` | null | The picture itself. Assign a `QImage` from C++, or bind to a `QImage` property of a C++ object, to show pixels that never touch a file; `source` becomes empty. Reading returns whatever is shown, loaded or assigned; a null image clears the item. |
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

### Dynamic pictures from C++

Two ways to show pixels the application produces itself, such as a spectrogram
or a decoded video frame:

- Assign `image`. Each assignment is one repaint, `source` is cleared, and a
  frame of the same size does not disturb the layout. Bind it to a `QImage`
  property of a C++ object to keep the QML declarative.
- Point `source` at `image://<provider>/<id>` and install a
  `qtui::ImageProvider` on the engine with `ImageProvider::add()`. The id is
  everything after the provider name, query included, so a counter in the
  query (`?frame=` + tick) asks C++ for a new picture. The provider receives
  the item's current pixel grid as the requested size and may render to it.

Frequent updates are cheap in the glyph modes; kitty re-uploads and sixel
re-encodes the picture on every change, so a stream of frames is best shown
with `Image.HalfBlock` or `Image.Braille`. `examples/concepts/live_image`
shows both shapes side by side, a scope trace bound through `image` and a
radar sweep served by a provider.

```qml
Image {
    width: 40; height: 12
    image: scope.frame          // a QImage property on a C++ object
    renderMode: Image.Braille
}
```

### Example

```qml
import QtQml
import Qtui
import Qtui.Image

Terminal {
    // _TODO_ show Image in use
}
```
