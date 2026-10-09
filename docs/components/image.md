---
title: Image
kind: qml
import: Qtui.Image
since: "0.4"
inherits: Item
cpp: qtui::Image
category: display
summary: Draws a raster image into terminal cells, with real pixels where the terminal supports them.
---

# Image QML type

Draws a raster image into terminal cells, with real pixels where the terminal supports them. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Image` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `source` | `url` | empty | Where the picture comes from: a local file, a `qrc:` resource, a path relative to the working directory, or an `http:`/`https:` URL. Any format Qt's image plugins read, including PNG, JPEG, and WebP. Remote pictures are fetched asynchronously through the QML engine's network access manager (redirects followed), the item stays `Loading` with `progress` advancing until the reply arrives, and decoded pictures are cached by URL so a second item or a relayout does not download again. Also `image://<provider>/<id>`, served by a `qtui::ImageProvider` installed on the QML engine; changing the URL asks the provider again. Cleared when `image` is assigned. |
| `image` | `QImage` | null | The picture itself. Assign a `QImage` from C++, or bind to a `QImage` property of a C++ object, to show pixels that never touch a file; `source` becomes empty. Reading returns whatever is shown, loaded or assigned; a null image clears the item. |
| `fillMode` | `FillMode` | `Image.Stretch` | How the picture is fitted into the item's cell rectangle. |
| `renderMode` | `RenderMode` | `Image.Auto` | How pixels become terminal output: a pixel protocol when the terminal has one, otherwise a glyph approximation. |
| `status` | `Status` | `Image.Null` | Read-only. Whether the picture loaded. |
| `progress` | `real` | `0.0` | Read-only. How much of a remote source has arrived, `0.0` to `1.0`; `1.0` once any picture is ready and `0.0` while nothing is shown. Local and resource pictures jump straight to `1.0`. |
| `sourceSize` | `size` | `0x0` | Read-only. The picture's size in pixels before scaling. |
| `renderSize` | `size` |  | Read-only. The pixel grid this item paints into: columns and rows times the cell's pixel size when kitty or sixel will draw, otherwise the glyph grid (1x2 per cell for half-block, 2x4 for Braille, 1x1 for ASCII). Produce frames at this size and they are shown without scaling; it follows the item size, `renderMode` and the terminal's capabilities. |
| `smooth` | `bool` | `true` | Filter the picture when scaling instead of picking the nearest pixel. With kitty, a source smaller than its placement is pre-scaled to the placement's pixel size so the terminal's own magnification cannot turn pixels into blocks; false uploads the source as is, which suits pixel art. |
| `alignment` | `Qt::Alignment` | `Qt.AlignCenter` | Where a picture smaller than the item sits inside it. |

## Enumeration Image.FillMode

| Value | Description |
| :--- | :--- |
| `Image.Stretch` | Scale to fill the item exactly, changing the aspect ratio. |
| `Image.PreserveAspectFit` | Scale uniformly so the whole picture fits, leaving the item's background showing on two sides. |
| `Image.PreserveAspectCrop` | Scale uniformly so the picture covers the item, cropping the overflow around the centre. |
| `Image.Tile` | Repeat the picture at its source size across the item. |
| `Image.Pad` | Draw at source size without scaling, placed by `alignment`. |

## Enumeration Image.RenderMode

| Value | Description |
| :--- | :--- |
| `Image.HalfBlock` | Two pixels per cell in true colour: the upper pixel is the foreground of a `▀` glyph and the lower pixel is its background. Works on every terminal. |
| `Image.Braille` | Eight dots per cell in one colour; a pixel brighter than the dark cut-off sets a dot. Best for line art on a dark background. |
| `Image.ASCII` | One pixel per cell, brightness mapped onto the ramp `` .:-=+*#%@`` and coloured with the pixel. |
| `Image.Kitty` | Real pixels through the kitty graphics protocol. Falls back to `HalfBlock` when the terminal does not support it. |
| `Image.Sixel` | Real pixels through sixel, as in Windows Terminal, xterm, and foot. Falls back to `HalfBlock` when the terminal does not support it. |
| `Image.Auto` | Kitty where available, then sixel, otherwise `HalfBlock`. |

## Enumeration Image.Status

| Value | Description |
| :--- | :--- |
| `Image.Null` | No `source` is set. |
| `Image.Ready` | The picture loaded and paints. |
| `Image.Loading` | The picture is being read or downloaded. Local reads are synchronous, so for them this is only observable from a `status` binding during the change; a remote source stays here until its reply arrives, keeping the previous picture on screen. |
| `Image.Error` | The `source` could not be read or decoded. Nothing paints. |

## Detailed Description

Image places a picture on the cell grid. The item's `width` and `height` are
cells, like every Qtui item; a half-block picture therefore has twice as many
pixel rows as the item has cell rows, and the implicit size is the source size
with the height halved. Set `fillMode` to keep the aspect ratio and `alignment`
to place the picture inside a larger item.

`renderMode` decides what a pixel becomes. On a terminal with a pixel protocol,
`Image.Auto` draws real pixels and the cells underneath are reserved so no text
paints through. Everywhere else the picture is approximated with glyphs, and
the same picture in `Image.HalfBlock` is what the headless test backend and the
committed-frame tests see.

The item repaints when the source, the geometry, or any rendering property
changes. Scaling is cached between frames, so a static picture costs nothing
after the first paint.

### Transparency

A terminal cell has no alpha channel: it holds one glyph, one opaque foreground
and one opaque background. Image applies one rule to every pixel with alpha so
that soft edges, shadows, and cut-outs behave predictably on any background.

- **Zero alpha draws nothing.** The cell keeps its glyph and colours, so
  whatever is behind the image shows through.
- **Partial alpha blends against what the cell holds.** The pixel is
  composited source-over onto the cell's background colour and the result is
  stored opaque. A half-transparent red over a blue panel is purple, exactly as
  it would be on a pixel display.
- **Full alpha replaces.** The pixel colour is written as it is.

In `Image.HalfBlock` the rule applies to each half of the cell. When only the
lower pixel is painted the glyph is `▄` rather than `▀`, so the unpainted half
shows the background behind the image and never a stale foreground colour.
`Image.ASCII` and `Image.Braille` pick their glyph from the composited colour:
a faint pixel over a dark background is a dim character, or no dot at all.

Blending needs a known background. The cell holds one whenever a
[Terminal](terminal.md) `color`, a [Rectangle](rectangle.md), or another item
painted there first. ANSI palette colours such as `"blue"` blend through their
xterm default RGB. When the cell still has the terminal's default background,
its real colour is unknown, so pixels at least half opaque are drawn unblended
and fainter pixels are left alone.

Pixel protocols keep the same contract by other means. Kitty uploads the PNG
with its alpha channel and the terminal composites it. Sixel has no alpha, so
pixels at least half opaque are drawn and the rest are not.

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
  the item's `renderSize` as the requested size and may render to it. An
  `AsyncImageProvider` answers with an `ImageResponse` that finishes later;
  the item shows its current picture while `status` is `Loading`, so frames
  rendered on another thread arrive without flicker.

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
    id: root
    color: "#0f172a"

    Rectangle {
        x: 2; y: 1
        width: 40; height: 12
        color: "#1e3a5f"
        border.style: "single"

        // A logo with a transparent background sits on the panel colour;
        // its soft edges blend into the blue instead of turning black.
        Image {
            anchors.fill: parent
            anchors.margins: 1
            source: "logo.png"
            fillMode: Image.PreserveAspectFit
            renderMode: Image.Auto
        }
    }

    Text {
        x: 2; y: 14
        text: root.capabilities.kittyGraphics ? "kitty: real pixels" : "half-block fallback"
        color: "#94a3b8"
    }
}
```
