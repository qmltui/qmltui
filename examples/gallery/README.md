# QmlTui Gallery

An interactive component catalog for every user-facing type in `QmlTui`,
`QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView` (45 components when
Private Qt `Behavior`, `QmlTui.Image`, and `QmlTui.PtyView` are enabled),
ordered from core primitives to controls and optional modules by frequency of
use.

- **Component `GridView` with Previews & On-Demand Header Search**: Browse all
  components in a virtualized `GridView` showing each component's name, module
  badge (`Core`, `Controls`, `Image`, `PtyView`), glyph preview content, and
  description blurb. Press `/` or click `/ Search` on the `Panel` top border to
  reveal and focus the inline search `TextInput` and filter components in real
  time.
- **Full-Width Interactive Example Container**: Press `Enter`, `Space`, or click
  a tile to open the full-width Example container `Panel` hosting the live
  interactive component example (centered for compact items, panel-filling for
  views and surfaces). Press `Escape` or click `[◂ Grid Esc]` in the top bar to
  return to the grid.

## Command-Line Options

```bash
# Run the full interactive gallery grid
./build/examples/gallery/qmltui_gallery

# List all available example names
./build/examples/gallery/qmltui_gallery --list-examples

# Launch a single example directly in standalone mode
./build/examples/gallery/qmltui_gallery --example Button
./build/examples/gallery/qmltui_gallery --example table_view

# Open the gallery directly on a specific component example
./build/examples/gallery/qmltui_gallery --page Button.qml
```

## Layout

- `examples/*.qml`: minimalistic, representative, commented QML examples focused
  solely on the target component (no outer container `Panel` or boilerplate).
- `Main.qml`: the Gallery shell hosting the searchable `GridView` (with the
  on-demand `Panel.titleTrailing` search input) and the full-width example
  stage.
- `scenarios/*.js`: deterministic `Runner` scenarios for each component example
  in `examples/*.qml`, used for both smoke verification and automated
  `.cast` / `.gif` recording.

## In the Browser (WebAssembly Playground)

`examples/wasm` and the documentation site's `/playground` page pair a live
CodeMirror 6 QML editor on the left (with context-aware borderless QML
autocomplete) with an `xterm.js` terminal on the right (`examples/wasm/www/index.html`).
You can either write a complete QML application with a root `Terminal {}` node
(supporting both `Terminal.AltScreen` and `Terminal.Inline` with
`Scrollback.print()`) or browse the Gallery Grid and live-edit individual
component examples in the browser. See `docs/guides/webassembly.md`.
