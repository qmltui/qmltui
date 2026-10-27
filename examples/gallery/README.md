# QmlTui Gallery

An interactive component catalog and live-editable QML playground for every
user-facing type in `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and
`QmlTui.PtyView` (37 components when Private Qt `Behavior`, `QmlTui.Image`, and
`QmlTui.PtyView` are enabled), ordered from core primitives to controls and
optional modules by frequency of use.

- **Component `GridView` with Previews & On-Demand Header Search**: Browse all
  components in a virtualized `GridView` showing each component's name, module
  badge (`Core`, `Controls`, `Image`, `PtyView`), glyph preview content, and
  description blurb. Press `/` or click `/ Search` on the `Panel` top border to
  reveal and focus the inline search `TextInput` and filter components in real
  time.
- **Gallery-Owned Example Container & On-Demand Live Editable Code Panel**: Press
  `Enter`, `Space`, or click a tile to open the full-width Example container
  `Panel` hosting the live interactive component example. Press `Ctrl+E` (or click
  `[Code ^E]`) to toggle the side-by-side split view with the editable QML Code
  Panel on the right:
  - **Left Pane**: The Gallery-owned container `Panel` hosting the live
    interactive component example (centered for compact items, panel-filling for
    views and surfaces).
  - **Right Pane** (`Ctrl+E`): An editable `TextArea` displaying the QML source of
    the active example with QML syntax highlighting. When you stop typing for
    `500 ms`, the Gallery automatically reloads the live preview from your
    edited QML source (`Ctrl+E` toggles the code pane; `Ctrl+Y` or `y` copies
    selected or full QML code to the clipboard; `Ctrl+T` switches focus between
    preview and code; `Ctrl+R` resets the source).

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
  on-demand `Panel.titleTrailing` search input) and the example stage with an
  on-demand split `[Example Panel | Editable Code Panel]` view (`Ctrl+E`) that
  compiles and reloads edited QML via `Qt.createQmlObject` after a `500 ms`
  debounce timeout and copies QML source to the clipboard (`Ctrl+Y`).
- `scenarios/*.js`: deterministic `Runner` scenarios for each component example
  in `examples/*.qml`, used for both smoke verification and automated
  `.cast` / `.gif` recording.

## In the Browser

`examples/wasm` and the documentation site's `/playground` page embed this
gallery: the gallery runs in xterm.js on the right, and opening a tile (or
following an `"▶ Open in Playground"` link from the docs) loads that tile's
file in a CodeMirror 6 editor on the left. See `docs/guides/webassembly.md`.
