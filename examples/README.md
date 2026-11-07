# Examples

Configure with `-DQMLTUI_BUILD_EXAMPLES=ON` to build the examples, and add `-DQMLTUI_BUILD_TESTS=ON` to register their `Runner` scenarios with CTest.

## Directory Layout

- **[`gallery/`](gallery/)**: Interactive component gallery (`qmltui_gallery`). `Main.qml` is the grid shell and [`gallery/examples/`](gallery/examples/) contains self-contained `Item`-rooted demos for the built-in types in `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
  ```sh
  ./build/examples/gallery/qmltui_gallery                   # full gallery grid
  ./build/examples/gallery/qmltui_gallery --playground      # open the live QML playground
  ./build/examples/gallery/qmltui_gallery --page Button.qml # open a specific tile
  ./build/examples/gallery/qmltui_gallery --example Button  # run one demo standalone
  ./build/examples/gallery/qmltui_gallery --list-examples   # list all demo names
  ```
- **[`recipes/`](recipes/)**: Standalone multi-file QML components, C++ `Item` and `SyntaxHighlighter` subclasses, and C++ `QAbstractItemModel` / `QSortFilterProxyModel` integrations.
- **[`helloworld/`](helloworld/)**: Minimal 3-file (`CMakeLists.txt`, `main.cpp`, `Main.qml`) standalone template using `FetchContent` or `find_package(QmlTui)`.
- **[`wasm/`](wasm/)**: Browser-hosted WebAssembly playground sharing the gallery demos (see [`docs/guides/webassembly.md`](../docs/guides/webassembly.md)).

See [`docs/examples.md`](../docs/examples.md) for the full catalog with links to QML type reference pages and recorded GIFs.

## Recipes (`examples/recipes/`)

| Directory | CMake Target | Summary |
| :--- | :--- | :--- |
| [`big_text/`](recipes/big_text/) | `qmltui_big_text_demo` | Custom C++ `qmltui::Item` (`BigText`) painting 3×5 block-pixel text into `TextGridBuffer`. |
| [`calendar/`](recipes/calendar/) | `qmltui_calendar_demo` | Pure-QML `Calendar.qml` month-grid component with keyboard navigation and date notes. |
| [`cpp_only/`](recipes/cpp_only/) | `qmltui_cpp_only_demo` | Constructing a `qmltui::Terminal` and item tree in C++ without `.qml` files. |
| [`custom_cpp_item/`](recipes/custom_cpp_item/) | `qmltui_custom_cpp_item_demo` | Minimal `qmltui::Rectangle` subclass (`ProgressBar`) exposed to QML with `QML_ELEMENT`. |
| [`independent_states/`](recipes/independent_states/) | `qmltui_independent_states_demo` | Orthogonal state machines using `StateGroup`, `State`, and `Transition` (`QMLTUI_ENABLE_PRIVATE_QT=ON`). |
| [`inline_repl/`](recipes/inline_repl/) | `qmltui_inline_repl_demo` | Inline CLI prompt using `Terminal.Inline` and `Scrollback.print()`. |
| [`lazy_model/`](recipes/lazy_model/) | `qmltui_lazy_model_demo` | `LazyDirectoryModel` (`QAbstractItemModel`) populating child nodes on demand in `TreeView`. |
| [`live_image/`](recipes/live_image/) | `qmltui_live_image_demo` | Streaming procedural `QImage` frames from `qmltui::AsyncImageProvider` into `Image` (`QMLTUI_ENABLE_IMAGE=ON`). |
| [`pyside6/`](recipes/pyside6/) | *(Python script)* | Hosting `QmlTui` and `QmlTui.Controls` from `PySide6` with a Python `QAbstractTableModel` and `QSortFilterProxyModel`. |
| [`standard_item_delegate/`](recipes/standard_item_delegate/) | `qmltui_standard_item_delegate_demo` | Shared `QStandardItemModel` rendered with `StandardItemDelegate` in `ListView`, `TableView`, and `TreeView` (plus `qmltui_standard_item_delegate_widgets` when `Qt6::Widgets` is available). |
| [`status_badge/`](recipes/status_badge/) | `qmltui_status_badge_demo` | Copyable pure-QML status pill (`StatusBadge.qml`). |
| [`table_view/`](recipes/table_view/) | `qmltui_table_view_demo` | `TableView` backed by `QAbstractTableModel` and `QSortFilterProxyModel`. |
| [`text_area_highlighting/`](recipes/text_area_highlighting/) | `qmltui_text_area_highlighting_demo` | Custom `TextArea` syntax highlighting in C++ (`JsonHighlighter`) and QML (`QmlHighlighter`). |
