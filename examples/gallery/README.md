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

- `examples/*.qml`: one `Item`-rooted component inspector per tile (Live
  Component Stage + Property Inspector & Telemetry), loaded directly by
  `Main.qml`'s `Loader` or launched standalone via `qmltui_gallery --example <Name>`.
- `Main.qml`: the master-detail component explorer, styled with the same brand
  palette (`#0d1117`, `#161b22`, `#30363d`, `#2cde85`, `#00ff66`) as the
  VitePress documentation site. It loads the selected component inspector via a
  declarative `Loader` (`source: "examples/" + current.file`), switches to
  `Terminal.Inline` when `InlineMode.qml` is opened, and hides the master
  header/sidebar/footer when launched in standalone `--example` mode.
- `scenarios/*.js`: deterministic `Runner` scenarios for each component
  example in `examples/*.qml`, used for both `AgentSmoke` verification and
  automated `.cast` / `.gif` recording.

## In the Browser

`examples/wasm` and the documentation site's `/playground` page embed this
gallery: the gallery runs in xterm.js on the right, and opening a tile (or
following an `"▶ Open in Playground"` link from the docs) loads that tile's
file in a CodeMirror 6 editor on the left. See `docs/topics/webassembly.md`.
