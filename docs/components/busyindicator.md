---
title: BusyIndicator
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Text
cpp: qtui::Text
category: display
summary: A one-cell animated spinner that cycles through a fixed sequence of glyphs while an operation is in progress.
---

# BusyIndicator QML type

A one-cell animated spinner that cycles through a fixed sequence of glyphs while an operation is in progress. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Text](text.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `style` | `int` | `BusyIndicator.Style.Dots` | Holds the glyph sequence: `BusyIndicator.Style.Dots` (braille spinner), `Line` (`|/-\`), `Pipe` (rotating box-drawing corner), `Block` (rotating quadrant) or `Bounce` (a `=` moving inside `[ ]`). |
| `running` | `bool` | `true` | Whether the animation advances; the current glyph stays visible when false. |
| `interval` | `int` | `100` | Holds the time in milliseconds each glyph is shown before the next one. |
| `_frameIndex` | `int` | `0` | Internal. Holds the animated frame counter that selects the current glyph. |
| `_frames` | `list<string>` |  | Read-only. Internal. Holds the glyph sequence selected by `style`. |
| `_currentSymbol` | `string` |  | Read-only. Internal. Holds `_frames[_frameIndex % _frames.length]`, the glyph bound to `text`. |

## Enumeration BusyIndicator.Style

| Value | Description |
| :--- | :--- |
| `BusyIndicator.Style.Dots` | A braille spinner, one cell wide. |
| `BusyIndicator.Style.Line` | The classic `|`, `/`, `-`, `\` rotation. |
| `BusyIndicator.Style.Pipe` | A box-drawing corner that rotates through the four orientations. |
| `BusyIndicator.Style.Block` | A quadrant block that rotates through the four corners. |
| `BusyIndicator.Style.Bounce` | A `=` that moves back and forth inside `[ ]`, six cells wide. |

## Detailed Description

BusyIndicator is a [Text](text.md) whose `text` is bound to the glyph at
`_frameIndex` in the sequence chosen by `style`. It is bold and `color`
defaults to `"default"`, the terminal's own foreground; set `color` to tint
it. The `Dots`, `Line`, `Pipe` and `Block` styles are one cell wide, `Bounce`
is six cells wide (`[=   ]`), and the item sizes itself like any `Text`.

The animation is a looping `NumberAnimation` on `_frameIndex` from `0` to the
number of frames in the sequence, with a duration of `interval` multiplied by
the frame count, so every glyph is shown for `interval` milliseconds and one
full turn takes `interval * _frames.length` milliseconds (1000 ms for the ten
braille frames at the default interval). It runs while both `running` and
`visible` are true, so hiding the indicator also stops the timer. Because
`_frameIndex` is an integer, the animated value is truncated and the index is
wrapped with a modulo, so the sequence repeats without a gap.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    property bool loading: true

    Timer {
        interval: 3000
        running: true
        onTriggered: root.loading = false
    }

    Row {
        anchors.centerIn: parent
        spacing: 1

        BusyIndicator {
            style: BusyIndicator.Style.Dots
            running: root.loading
            color: "cyan"
            visible: root.loading
        }
        Text {
            text: root.loading ? "Fetching results" : "Done"
        }
    }
}
```
