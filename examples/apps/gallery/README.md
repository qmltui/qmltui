# Qamui Gallery

A full-screen grid of every user-facing type in `Qamui` and `Qamui.Controls`,
39 tiles in all with Behavior and Qamui.Image built in, 36 without. Move the selection with the arrow keys (or `hjkl`), open a
tile with Enter, Space or a mouse click. Escape returns to the grid; Q quits
from the grid or from a demo that has no text input focused.

## Layout

- `examples/*.qml`: one complete, standalone app per tile, with its own
  imports and `Terminal` root. Each runs on its own (`qml`-style single file,
  or paste it into the browser page) and is what the gallery shows.
- `Main.qml`: the launcher. It receives the example texts through the
  `qamuiExampleSources` context property (a JSON object `{ "Button.qml":
  "<qml>", ... }`), rewrites the root `Terminal {` of the chosen file into an
  `Item` that fills the demo panel, drops the root-level properties only a
  Terminal has (`title`, `color`, `mode`, the quit handler), and instantiates
  the result with `Qt.createQmlObject`. A root `mode: Terminal.Inline` makes
  the launcher itself switch to inline mode, so `InlineMode.qml` shows the
  normal-buffer mode inside the gallery too.
- `main.cpp` reads `examples/*.qml` from the resource and sets the context
  property; the browser page passes what its editor holds.

Because the launcher only reads text, an example is exactly what runs.

## In the browser

`examples/wasm` builds the site around this app: the gallery runs in xterm.js
on the right, and opening a tile puts that tile's file in a CodeMirror editor
on the left. Every edit is compiled first (`qamuiCheckQml`), then the gallery
re-runs with the same tile open (`qamuiGalleryPage`); Escape empties the
editor again, and "launcher" edits `Main.qml`. The launcher names the open
file in its terminal title (`Qamui Gallery · Button.qml`), which is how the
page follows it. See `docs/guide/webassembly.md`.

## Tests

`test_gallery.js` is the Runner scenario (`ctest -R gallery_smoke` in a
tree with `QAMUI_ENABLE_RUNNER=ON`): it walks the grid with the keyboard,
opens the Button demo and activates a button, clicks the Slider tile, checks
Escape closes a Dialog before leaving its page, types into a TextInput, opens
every example once so a broken file fails the build, and finally round-trips
through inline mode.
