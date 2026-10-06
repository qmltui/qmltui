# Qtui 0.4 Migration Notes

Qtui 0.4 changes the rendering hot path. As with 0.3, there is no
compatibility shim. The compiler reports source API changes; the layout and
scrolling behavior changes below also need review because they can compile
without preserving the old behavior.

## Cell glyph storage

`Cell::character` is no longer a public `QByteArray` field. The cluster bytes
are stored inline in a 24-byte, trivially copyable cell (long clusters are
interned process-wide) and accessed through methods:

| 0.3 | 0.4 |
|-----|-----|
| `cell.character = "X";` | `cell.setCharacter("X");` |
| `cell.character = text.toUtf8();` | `cell.setCharacter(text.toUtf8());` |
| `cell.character += marks;` | `cell.appendCharacter(marks);` |
| `cell.character.clear();` | `cell.clearCharacter();` |
| `QString::fromUtf8(cell.character)` | `QString::fromUtf8(cell.character())` |
| `cell.character.isEmpty()` | `cell.character().isEmpty()` |
| `cell.character == "X"` | `cell.character() == "X"` |
| `QCOMPARE(cell.character, QByteArray("X"))` | `QCOMPARE(cell.character().toByteArray(), QByteArray("X"))` |
| `cell.character.data()` | `cell.character().data()` (not NUL-terminated; use the size) |

`character()` returns a `QByteArrayView` that is valid until the cell is
modified or its buffer is resized, cleared, swapped or destroyed. Copy it with
`toByteArray()` when it must outlive the cell.

`Cell` is no longer an aggregate; construct it with `Cell{}` or `Cell cell;`
and assign fields afterwards.

Long-cluster storage is append-only: displaying the same cluster repeatedly
reuses one entry, but distinct clusters longer than 12 UTF-8 bytes remain until
process exit, even after their cells are cleared. This preserves value copies
and borrowed glyph views without reference-counted cells. Qtui 0.4 retains this
tradeoff; it does not guarantee bounded glyph memory for applications displaying
an unlimited variety of long combining or ZWJ sequences. Applications handling
such streams should limit accepted cluster length/variety or isolate that
workload in a restartable process. Evicting table entries while cells can still
reference them would corrupt live and captured frames.

## paint() and paintOverlay() are const

Add `const` to every `paint(...) override` and `paintOverlay(...) override`.
Anything the item computes lazily while painting (wrapped lines, scaled images)
must live in a `mutable` member; property changes belong in setters or
`updateLayout()`. Canvas shapes now implement
`draw(CanvasPainter&) const` and draw through the painter instead of the
`Canvas*`; `CanvasShape::resolvedColor()` takes the painter as well.

## Layout and paint phases

Each frame runs a layout pass (`updateLayout()`) over the whole tree before
the paint pass. Geometry changed in `updateLayout()` is painted in the same
frame, and `setWidth()`/`setHeight()` mark an item `LayoutDirty` so containers
relayout when their own size changes.

Property changes made from `paint()` or `paintOverlay()` are retained but no
longer schedule a frame. Run with `QTUI_DEBUG_PAINT_DIRTY=1` to have each
offending item reported once, then move the change into `updateLayout()` or a
property setter.

Layout-dependent setters must request `LayoutDirty | SelfDirty`, including
changes to providers that calculate child geometry. Stream renderers resolve
layout before sizing the prompt buffer, so an implicit-height change is visible
in the same committed frame.

## Terminal colour depth and encoder

Frames are now encoded for the detected colour depth (`TerminalCapabilities`,
installed header `qtui/primitives/TerminalCapabilities.h`). A terminal with
`TERM` containing `256color` and no `COLORTERM` receives 256-colour SGR
sequences instead of 24-bit ones; set `QTUI_COLOR_DEPTH=truecolor` to force the
old output. `TextGridBuffer::diff()` gained overloads that append into a
caller-owned `std::string` with `AnsiEncodeOptions`, and clearing the tail of a
row now emits erase-to-end-of-line (`\033[K`) instead of a run of blanks.

## Content offsets

`Item::contentOffset()` no longer reads dynamic `contentX`/`contentY`
properties; only `ScrollView` and its subclasses scroll their children. A QML
`property int contentY` declared on a plain `Item` has no effect on child
positions any more. Derive from `ScrollView` or override `contentOffset()` in
a custom C++ item instead.

## Build integration

Use `find_package(Qtui 0.4 REQUIRED)`.

## Output shutdown

`TerminalDevice::setAsyncOutput(false, timeoutMs)` returns whether disabling
succeeded. A timeout preserves the queue, notifier, and non-blocking descriptor
so delivery can resume. `flushBlocking(timeoutMs)` uses one deadline for the
whole flush, including partial writes and interrupted waits.

For final teardown, `finishOutput(timeoutMs)` queues cursor/style restoration
and attempts to deliver everything within that deadline. It always stops the
notifier and attempts to restore descriptor flags; on failure it returns false
and discards the undelivered tail. The engine emits `renderFailed` after restoring
input mode, allowing the application to report failure without another write to
the stalled output descriptor. Further writes are
rejected until asynchronous output is enabled again, avoiding a blocking write
from a repeated stop or destructor. No implementation can guarantee delivery to
a terminal that has stopped reading; raw input mode is still restored.
Direct users of `TerminalDevice` should call `finishOutput()` to observe delivery
failure. Its destructor performs best-effort cleanup without logging, because
stderr can share the same stalled terminal.
