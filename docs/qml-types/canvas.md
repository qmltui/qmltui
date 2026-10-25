---
title: Canvas
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::Canvas
category: canvas
summary: Provides a sub-cell graphical drawing surface using Braille or half-block characters.
---

# Canvas QML type

Provides a sub-cell graphical drawing surface using Braille or half-block characters. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `color` | `color` | `"default"` | Holds the default drawing and outline color for shapes and pixels drawn on the canvas. |
| `gradient` | `Gradient` | `null` | Holds an optional [Gradient](gradient.md) used across the canvas when shapes or pixels do not specify their own color or gradient. |
| `fillColor` | `color` |  | Holds the background fill color of the canvas item. |
| `dim` | `bool` | `false` | Holds whether canvas cells render with the terminal dim/faint (`SGR 2`) attribute by default. |
| `renderMode` | `RenderMode` | `Canvas.Braille` | Holds the sub-cell rendering mode: `Canvas.Braille` (2x4 dots per cell) or `Canvas.HalfBlock` (1x2 blocks per cell). |
| `cellColorMode` | `CellColorMode` | `Canvas.LastDot` | Holds how multi-colored dots sharing the same Braille cell resolve cell color: `Canvas.LastDot` or `Canvas.Average`. |
| `autoClear` | `bool` | `true` | Holds whether the pixel buffer is automatically cleared before each paint pass. |
| `pixelWidth` | `int` |  | Read-only. Holds the total width in sub-cell canvas pixels (`width * 2` for Braille, `width` for HalfBlock). |
| `pixelHeight` | `int` |  | Read-only. Holds the total height in sub-cell canvas pixels (`height * 4` for Braille, `height * 2` for HalfBlock). |

## Methods

| Method | Description |
| :--- | :--- |
| `void setPixel(int x, int y, bool value)` | Sets or clears the pixel at coordinates (`x`, `y`). |
| `void setPixel(int x, int y)` | Turns on the pixel at coordinates (`x`, `y`) using the canvas `color`. |
| `void setPixelColor(int x, int y, Color color)` | Sets the pixel at (`x`, `y`) to the specified `color`. |
| `void clear()` | Clears all pixels from the internal canvas buffer. |
| `void requestPaint()` | Schedules a repaint of the canvas for the next frame. |

## Enumeration Canvas.RenderMode

| Value | Description |
| :--- | :--- |
| `Canvas.Braille` | Renders graphics using Unicode Braille Patterns (U+2800..U+28FF), providing 2x4 sub-cell dot resolution per character cell. |
| `Canvas.HalfBlock` | Renders graphics using upper and lower half-block characters (▀ and ▄), providing 1x2 sub-cell resolution with dual-color foreground and background. |

## Enumeration Canvas.CellColorMode

| Value | Description |
| :--- | :--- |
| `Canvas.LastDot` | The foreground color of a Braille cell is determined by the most recently drawn dot in that cell. |
| `Canvas.Average` | The foreground color of a Braille cell is computed as the arithmetic RGB average of all colored dots drawn in that cell. |

## Detailed Description

Canvas provides sub-cell graphical drawing capabilities in the terminal without
requiring a graphical display server or terminal graphics protocol. By using
Unicode Braille characters (which provide a 2x4 dot matrix per text cell) or
half-block characters (providing a 1x2 block matrix per cell), Canvas allows
smooth rendering of lines, charts, curves, and 2D geometric shapes.

Declarative child shapes such as [Line](line.md), [Circle](circle.md),
[Point](point.md), [Polygon](polygon.md), and [CanvasRectangle](canvasrectangle.md)
are automatically drawn onto the canvas during its paint pass. In addition,
procedural drawing can be performed from JavaScript handlers using `setPixel()`,
`setPixelColor()`, and `clear()`.

Coordinates for shapes and pixels are specified in canvas sub-cell pixels,
bounded by `pixelWidth` and `pixelHeight`.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Canvas {
        anchors.centerIn: parent
        width: 30
        height: 10
        renderMode: Canvas.Braille
        color: "cyan"

        Circle {
            cx: 30
            cy: 20
            radius: 12
            color: "yellow"
        }

        Line {
            x1: 0
            y1: 0
            x2: 59
            y2: 39
            color: "magenta"
        }
    }
}
```

### Gallery Example

![Canvas recorded demo](../assets/casts/components/canvas.gif)

See [`examples/gallery/examples/Canvas.qml`](../../examples/gallery/examples/Canvas.qml) (`./build/examples/gallery/qmltui_gallery --example Canvas`, or open in the [WebAssembly Playground](../playground.md?example=Canvas.qml)).
