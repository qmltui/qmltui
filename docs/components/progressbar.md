---
title: ProgressBar
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: display
summary: A one-row horizontal bar that fills from left to right as a value moves from `from` to `to`.
---

# ProgressBar QML type

A one-row horizontal bar that fills from left to right as a value moves from `from` to `to`. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `value` | `real` | `0.0` | Holds the current value; it is clamped into the `from`..`to` range when the bar is drawn. |
| `from` | `real` | `0.0` | Holds the value at which the bar is empty. |
| `to` | `real` | `1.0` | Holds the value at which the bar is full; when `to` is not greater than `from` the bar stays empty. |
| `showBrackets` | `bool` | `false` | Whether the bar is enclosed in `[` and `]`, taking two cells from the bar width. |
| `showPercentage` | `bool` | `false` | Whether a five-cell suffix such as ` 42%` is drawn after the bar, taking five cells from the bar width. |
| `barWidth` | `int` |  | Read-only. Holds the number of cells left for the bar itself: the item width minus 2 for brackets and 5 for the percentage, never negative. |
| `percentageText` | `string` |  | Read-only. Holds the percentage suffix, a space and a right-aligned integer padded to three cells followed by `%`, or the empty string when `showPercentage` is false. |
| `style` | `string` | `"block"` | Holds the glyph preset: `"block"` draws `█`/`░`, `"segmented"` draws `▰`/`▱`, `"pill"` draws `▮`/`▯`, `"dot"` draws `●`/`○`, `"hash"` draws `#`/`-`; an unknown value draws like `"block"`. |
| `color` | `rgba` | `"default"` | Holds the colour of the bar, brackets and percentage when no gradient or thresholds are set, and of the empty portion and decorations when they are. |
| `fillCharacter` | `string` | `""` | Holds a glyph drawn for each filled cell; when non-empty it overrides `style`. |
| `emptyCharacter` | `string` | `""` | Holds a glyph drawn for each empty cell; when non-empty it overrides `style`. |
| `gradient` | `Gradient` | `null` | Holds a [Gradient](gradient.md) whose stops colour the filled cells by their position along the bar, `0.0` at the left edge and `1.0` at the right. |
| `thresholds` | `var` |  | Holds an array of `{ value, color }` objects, in ascending order of `value`, that colour the filled cells in `from`..`to` units; ignored while `gradient` has stops. |
| `hasGradient` | `bool` |  | Read-only. True when `gradient` has at least one stop or `thresholds` is non-empty, which switches the bar to per-segment colouring. |
| `effectiveFillChar` | `string` |  | Read-only. Holds the glyph actually used for filled cells after applying `fillCharacter` and `style`. |
| `effectiveEmptyChar` | `string` |  | Read-only. Holds the glyph actually used for empty cells after applying `emptyCharacter` and `style`. |
| `position` | `real` |  | Read-only. Holds `value` normalised into `0.0`..`1.0` within `from`..`to`, or `0.0` when `to` is not greater than `from`. |
| `segments` | `var` |  | Read-only. Holds the list of `{ text, color }` runs drawn in gradient mode, one per colour change from left to right plus one for the empty portion; empty otherwise. |

## Methods

| Method | Description |
| :--- | :--- |
| `var getColorAt(var pos)` | Returns the colour for a normalised position `0.0`..`1.0` along the bar: the colour of the first gradient stop whose `position` is at or beyond `pos`, else the first threshold whose value is at or beyond it, else `color`; past the last stop or threshold its colour is returned. |

## Detailed Description

The bar is a single row of glyphs, `implicitHeight` 1 and `implicitWidth` 15;
set `width` to choose its length. Of that width, `barWidth` cells are the bar
proper: `Math.round(position * barWidth)` cells are drawn with
`effectiveFillChar` and the rest with `effectiveEmptyChar`. `showBrackets`
wraps the bar in `[` and `]` and `showPercentage` appends `percentageText`, so
`" 100%"` at full width; both are taken out of `barWidth` rather than added to
the item. When the width leaves no cells for the bar, only the decorations are
drawn. Everything is drawn bold.

With neither a gradient nor thresholds the whole bar is one bold `Text` in
`color`. When `hasGradient` is true the bar is instead a `Row` of one `Text`
per entry in `segments`: for each filled cell `i` the colour is
`getColorAt(i / (barWidth - 1))`, adjacent cells with the same colour are
merged into one run, and the empty portion, brackets and percentage use
`color`. Colouring is stepped, not interpolated: a cell takes the colour of the
first `GradientStop` whose `position` is at or beyond the cell's position, or
of the first threshold whose `value` (in `from`..`to` units) is at or beyond
it, so stops and thresholds are expected in ascending order. A gradient with
stops takes precedence over thresholds.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    property real downloaded: 0

    NumberAnimation {
        target: root
        property: "downloaded"
        from: 0
        to: 100
        duration: 4000
        loops: -1
        running: true
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        ProgressBar {
            width: 40
            value: root.downloaded
            from: 0
            to: 100
            showBrackets: true
            showPercentage: true
            color: "cyan"
        }

        ProgressBar {
            width: 40
            value: root.downloaded
            to: 100
            style: "dot"
            thresholds: [{ value: 60, color: "green" }, { value: 85, color: "yellow" }, { value: 100, color: "red" }]
        }
    }
}
```
