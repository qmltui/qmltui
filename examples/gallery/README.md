# QmlTui Gallery

A full-screen grid of every user-facing type in `QmlTui`, `QmlTui.Controls`, and
`QmlTui.Image` (39 tiles with Private Qt `Behavior` and `QmlTui.Image` enabled,
36 without). Move the selection with the arrow keys (or `hjkl`), open a tile
with `Enter`, `Space`, or a mouse click, and press `T` in the grid to toggle
between the Dark and Light documentation themes. `Escape` returns to the grid;
`Q` quits from the grid or from a demo that has no text input focused.

## Command-Line Options

```bash
# Run the full interactive gallery grid
./build/examples/gallery/qmltui_gallery

# List all 39 standalone example names
./build/examples/gallery/qmltui_gallery --list-examples

# Launch a single example directly as a standalone Terminal app
./build/examples/gallery/qmltui_gallery --example Button
./build/examples/gallery/qmltui_gallery --example table_view

# Open the gallery shell directly on a specific component tile
./build/examples/gallery/qmltui_gallery --page Button.qml
```

## Layout

- `examples/*.qml`: one complete, standalone app per tile, with its own
  imports and `Terminal` root. Each runs on its own (`qmltui_gallery --example <Name>`,
  or in the browser playground) and is what the gallery instantiates.
- `Main.qml`: the launcher, styled with the same brand palette (`#0d1117`,
  `#161b22`, `#30363d`, `#2cde85`, `#00ff66`) as the VitePress documentation
  site. It receives the example texts through the `qmltuiExampleSources`
  context property (a JSON object `{ "Button.qml": "<qml>", ... }`), rewrites
  the root `Terminal {` of the chosen file into an `Item` that fills the demo
  panel, drops the root-level properties only a `Terminal` has (`title`,
  `color`, `mode`, the quit handler), and instantiates the result with
  `Qt.createQmlObject`. A root `mode: Terminal.Inline` makes the launcher
  itself switch to inline mode, so `InlineMode.qml` shows the normal-buffer
  mode inside the gallery too.
- `scenarios/*.js`: deterministic `Runner` scenarios for each standalone
  example in `examples/*.qml`, used for both `AgentSmoke` verification and
  automated `.cast` / `.gif` recording.

## In the Browser

`examples/wasm` and the documentation site's `/playground` page embed this
gallery: the gallery runs in xterm.js on the right, and opening a tile (or
following an `"▶ Open in Playground"` link from the docs) loads that tile's
file in a CodeMirror 6 editor on the left. See `docs/topics/webassembly.md`.
