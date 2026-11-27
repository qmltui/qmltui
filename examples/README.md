# Examples

Configure with `-DQMLTUI_BUILD_EXAMPLES=ON` to build the examples, and add `-DQMLTUI_BUILD_TESTS=ON` to register their `Runner` scenarios with CTest.

- **[`gallery/`](gallery/)**: Interactive component catalog (`./build/examples/gallery/qmltui_gallery`, 42 default demos; 44 with optional `Image` enabled). Each file in [`gallery/examples/`](gallery/examples/) is a standalone `Item`-rooted demo.
- **[`recipes/`](recipes/)**: Copyable multi-file QML components, custom C++ `Item` subclasses and `TextArea` highlighters, C++ `QAbstractItemModel` / `QSortFilterProxyModel` setups, and a `PySide6` host script (see [Python and PySide6](../docs/guides/python-pyside6.md)).
- **[`helloworld/`](helloworld/)**: Minimal 3-file (`CMakeLists.txt`, `main.cpp`, `Main.qml`) standalone starter supporting `FetchContent`, `find_package(QmlTui)`, and `qmltui_wasm_app()`.
- **WebAssembly**: Browser targets use `qmltui_wasm_app()`; see the [Qt for WebAssembly guide](../docs/guides/webassembly.md) and the live [WebAssembly Playground](https://qmltui.github.io/playground.html).

See [`docs/examples.md`](../docs/examples.md) for the complete gallery and recipe tables, CMake target names, CLI flags, QML type links, and recorded GIFs.
