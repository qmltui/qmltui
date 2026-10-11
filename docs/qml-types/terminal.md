---
title: Terminal
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Terminal
category: core
summary: _TODO_
---

# Terminal QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `capabilities` | `terminalCapabilities` |  | Read-only. _TODO_ |
| `title` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `mode` | `Mode` |  | _TODO_ |
| `clipboardMode` | `ClipboardMode` |  | _TODO_ |
| `mouseReporting` | `bool` |  | Whether the application takes the mouse from the terminal: on in `FullScreen`, off in `Inline` unless set explicitly, in which case the value sticks across mode changes. Off means no clicks, drags, wheel or hover reach any item and the terminal keeps selection and scrollback; hover requests (`MouseArea.hoverEnabled`, `ToolTip`) only turn on motion reporting while this is true. |
| `hoveredItem` | `Item` |  | Read-only. _TODO_ |
| `terminalWidth` | `int` |  | Read-only. _TODO_ |
| `terminalHeight` | `int` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `clipboardCopyRequested(string text)` | _TODO_ |
| `focusedItemChanged()` | _TODO_ |
| `mouseMotionRequestedChanged(bool requested)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void copyToClipboard(string text)` | _TODO_ |

## Enumeration Terminal.Mode

| Value | Description |
| :--- | :--- |
| `Terminal.FullScreen` | _TODO_ |
| `Terminal.Inline` | _TODO_ |

## Enumeration Terminal.ClipboardMode

| Value | Description |
| :--- | :--- |
| `Terminal.Disabled` | _TODO_ |
| `Terminal.Osc52` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    Text { text: "Hello" }
}
```

## Core element behavior

- `Terminal` derives from `Item` and owns the terminal session. `mode` is
  `Terminal.FullScreen` by default, or `Terminal.Inline`. It selects screen
  buffer, rendering, and sizing together, and can change at runtime.
- Read-only `Terminal.capabilities` exposes `kittyKeyboard`, `kittyGraphics`,
  `cellPixelSize`, and `syncOutput`. An asynchronous startup probe refines these
  values without delaying the first frame; changes notify QML bindings.
  When `kittyKeyboard` is true the engine has pushed the kitty keyboard
  protocol's disambiguate flag for the session: Escape arrives immediately,
  Ctrl and Alt chords carry their modifiers, and Ctrl+Enter, Shift+Enter and
  Ctrl+Shift+letter reach `Keys.onPressed` as distinct events. On a legacy
  terminal a lone Escape is delivered after a short window
  (`QAMUI_ESCAPE_TIMEOUT_MS`, default 50) and those chords collapse to their
  unmodified keys. `QAMUI_KITTY_KEYBOARD=0` keeps the legacy path on a terminal
  that answers the query but misbehaves. Graphics transport support does not
  imply Unicode placeholder support. `cellPixelSize` is invalid when unknown.
- `Image.renderMode` selects how pixels reach the terminal: `HalfBlock` (default
  glyph rendering), `Braille`, `ASCII`, `Kitty` (true pixels via the kitty
  graphics protocol using Unicode placeholders), and `Auto`, the new default.
  `Auto` uses kitty where the terminal supports it and true colour, and falls
  back to `HalfBlock` everywhere else, so the same QML shows real images in
  kitty, Ghostty, WezTerm and Konsole and glyph images in every other terminal.
  Sixel (`Sixel` mode) is also supported, for terminals such as Windows Terminal
  that implement sixel but not kitty; `Auto` prefers kitty, then sixel, then
  half-block. Kitty uploads are freed automatically when the source changes, the
  item is destroyed, or the session ends. Sixel paints at the cursor rather than
  into cells, so it is drawn after the cell diff and cannot sit under other cell
  content the way kitty placeholders can; use it for a dedicated image region.
- `Image.sourceClipRect` shows one region of the source, cropped before any
  scaling, so a sprite sheet or icon atlas is decoded once and each item crops
  its own tile. `AnimatedSprite` (also `Qamui.Image`) is Image plus a frame
  clock: it plays a frame grid out of one sheet by driving that clip, with the
  Qt Quick AnimatedSprite property set (`frameCount`, `frameWidth`/`frameHeight`,
  `frameX`/`frameY`, `frameRate`/`frameDuration`, `loops`, `reverse`,
  `finishBehavior`).
- All visible children render in ordinary stacking order in both modes. Use
  layouts to arrange them; overlapping children are not automatically stacked
  vertically or treated as pages. Visibility and focus remain application state.
- Full-screen roots fill the terminal. Inline roots use terminal width and
  visible content height, capped at terminal rows minus one cursor row. Hidden
  children do not contribute to height. Vertically filling a content-sized
  root is circular; give inline content an explicit or implicit height.
- `Scrollback` is nonvisual and never changes mode. `print(string)` returns
  whether output was accepted. `append(Item)` snapshots a borrowed item;
  `append(data)` renders a delegate without inserting into the bound model.
  Model inserts append rows, edits do not rewrite committed output, and resets
  start a new output sequence. `clear()` resets the delegate cache and consumed
  position; it does not erase the model, accepted output, or terminal history.
- Accepted scrollback belongs to the session, with a 1 MiB UTF-8 queue shared
  by its output objects. Full-screen mode defers output until inline resumes
  or the session stops. Oversized submissions are rejected in full, retaining
  earlier output; `outputRejected(reason)` and a warning report rejection.
  Output snapshots retain their submission-time formatting across resizing.
- Root `color` is the background. Borders and fixed dimensions belong on inner
  rectangles or panels. Nested `Terminal` roots are unsupported; switch normal
  child visibility and `mode` for a persistent full-screen page.
- `Item` is the visual base type and provides cell geometry, visibility, focus,
  clipping, anchors, and stacking order.
- `ScrollView` provides cell-based scrolling for content larger than its bounds.
- `Loader` creates one `Qamui.Item` from `source` or `sourceComponent`.
- `Row` and `Column` lay out children with integer spacing.
- `Text`, `TextInput`, and `TextArea` provide terminal text rendering and input.
- `MouseArea` provides click, wheel, hover, and drag input with the Qt Quick
  vocabulary: `acceptedButtons` (default `Qt.LeftButton`), the `pressed` bool
  alongside the signal, `pressedButtons`, `containsPress` for press feedback
  that needs no motion reporting, and an `onWheel` that consumes the event
  only while a handler is connected. `MouseEvent.button` carries Qt values
  (`Qt.LeftButton`, ...); the raw terminal code is `nativeButton`.

`Qamui.Controls` contains the controls a user operates or reads as finished
widgets, including `TabBar`, `Toast`, `Sparkline`, and `Popup`. `Canvas` and its shapes are part of `Qamui`. `StatusBadge`, `Calendar`,
and `BigText` are recipes under `examples/components/`.
