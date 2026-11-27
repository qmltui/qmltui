# QmlTui Gallery (`qmltui_gallery`)

Interactive catalog of 42 default interactive demos (44 when built with `-DQMLTUI_ENABLE_IMAGE=ON`, 41 with `-DQMLTUI_ENABLE_PRIVATE_QT=OFF`) covering all 74 built-in types across `QmlTui`, `QmlTui.Controls`, and `QmlTui.Image`. Press `/` to filter components, `Enter` or `Space` to open a live interactive demo, and `Escape` to return to the grid.

## Usage

```sh
./build/examples/gallery/qmltui_gallery                   # interactive component grid
./build/examples/gallery/qmltui_gallery --list-examples   # list available demo names
./build/examples/gallery/qmltui_gallery --example Button  # run a single demo standalone
./build/examples/gallery/qmltui_gallery --page Button.qml # open the grid on a specific tile
```

## Layout

- `Main.qml`: Searchable `GridView` shell and full-width example stage.
- `examples/*.qml`: Self-contained `Item`-rooted component examples (also runnable in the browser via `qmltui_wasm_app()`; see [`docs/guides/webassembly.md`](../../docs/guides/webassembly.md)).
- `scenarios/*.js`: Deterministic `Runner` smoke tests and `.cast` / `.gif` recording scripts.
