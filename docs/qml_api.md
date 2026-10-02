# QML API Reference

Qtui exposes a core subset of declarative UI elements, heavily styled after Qt Quick. This document outlines the available QML elements.

## Core Elements
- `Terminal`: The root window element for any Qtui application. Captures top-level events and dictates the rendering mode.
  - *Properties*: `fillTerminal` (bool), `useAlternateScreen` (bool), `rendererMode` (`Terminal.Frame` or `Terminal.Stream`).
  - *Methods*: `print(string)` (appends text in Stream mode), `append(Item)` (renders and appends a QML item in Stream mode).
- `StreamAppender`: Observes a `QAbstractItemModel` and appends inserted rows through a QML delegate, including `DelegateChooser`. Role names and values are application-defined. Existing rows are append-only; updates and removals do not rewrite streamed output.
- `Item`: The base visual element. All other elements inherit from it. Provides positioning and sizing logic.
  - *Properties*: `x`, `y`, `width`, `height`, `implicitWidth`, `implicitHeight`, `clip`, `visible`, `focusable`.
- `Text`: Renders colored text. Supports multiline wrapping.
  - *Properties*: `text` (string), `color` (string, e.g. "red", "#FF0000"), `attributes` (bitmask of `Text.Bold`, `Text.Underline`, etc.), `textFormat` (`Text.PlainText`, `Text.RichText`, `Text.AnsiText`, or `Text.AutoText`).
- `MouseArea`: Invisible component that intercepts mouse clicks and scrolls.
  - *Signals*: `onClicked`, `onWheelUp`, `onWheelDown`.

## Layouts & Positioning
- `Row`: Positions children horizontally. See [Layouts Demo](../examples/layouts_and_padding/Main.qml).
  - *Properties*: `spacing`.
- `Column`: Positions children vertically.
  - *Properties*: `spacing`.
- `Anchors`: Allows aligning elements relative to parents and siblings.
  - *Properties*: `anchors.fill`, `anchors.centerIn`, `anchors.left`, `anchors.right`, `anchors.top`, `anchors.bottom`, `anchors.margins`.

## Views & Repetition
- `Repeater`: Dynamically generates children based on a numeric count or data model.
  - *Properties*: `model`, `delegate`.
- `ListView`: A scrolling vertical list for data models. Supports scrolling with arrow keys or mouse wheel. See [ListView Demo](../examples/listview_demo/Main.qml).
  - *Properties*: `model`, `delegate`, `currentIndex`.
- `ScrollView`: Provides scrollable bounds for content larger than its visual rectangle. See [ScrollView implementation](../Tui/src/items/ScrollView.h).

## Inputs & Controls
- `TextInput`: A single-line text input field supporting cursors and backspace editing.
  - *Properties*: `text`, `color`, `cursorPosition`.
  - *Signals*: `onAccepted`, `onTextChanged`.
- `TextArea`: A multi-line text input field supporting vertical navigation and entering newlines.
- `Border`: Draws a border outline around an item, using ASCII or Unicode box drawing characters.

## Animations & States
- `State`: Defines a cluster of property overrides based on conditions.
- `Transition`: Smoothly animates property changes between states.
- `PropertyAnimation`: Linearly interpolates numeric values (e.g., `x`, `width`) over a duration. See [Animation Demo](../examples/animation_demo/Main.qml).
