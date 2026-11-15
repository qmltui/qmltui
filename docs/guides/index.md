# Guides

These guides cover how QmlTui maps Qt 6 concepts to the terminal, its retained-mode cell architecture, UI subsystems, C++ extensibility, and testing tools.

## Onboarding and Core UI

1. **[QmlTui for Qt Quick and Widgets Developers](qt-quick-to-qmltui.md)**: Differences from `QtQuick` and `QtWidgets`, including `QCoreApplication`, imports, integer character-cell geometry, box-drawing borders, colors, keyboard quit handling, and shared C++ models.
2. **[Architecture](architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` cell diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and QML module boundaries.
3. **[Layouts & Cell Geometry](layouts.md)**: Integer cell coordinates, borders and insets, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout`.
4. **[Input & Focus](input-and-focus.md)**: Focus chains, `FocusScope`, `Keys.onPressed` propagation, `Shortcut`, contextual `Escape` quit handling, and `MouseArea`.
5. **[Styling & Colors](styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient` / `GradientStop`, component styling, and `TextArea` highlight layers.
6. **[Models in QmlTui Views](models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView`.
7. **[Animation & States](animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `StateGroup`, `Transition`, and optional `Behavior`.

## Rendering Modes, C++ Extensibility, and Tooling

8. **[Inline Mode & Scrollback](inline-mode.md)**: Building streaming CLIs and REPLs with `Terminal.Inline` and `Scrollback`.
9. **[Embedded PTY Shells (`QmlTui.PtyView`)](pty-view.md)**: Hosting interactive Unix child processes inside `PtyView` on Linux and macOS.
10. **[Custom C++ Items](custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer`, registering QML types with `QML_ELEMENT`, and building C++-only scenes.
11. **[Testing & Recording with Runner](testing.md)**: Headless testing with a virtual clock, text and style snapshots, Asciinema `.cast` recording, and `qmltui-runner`.
12. **[Agentic Development Loop with Runner](agentic-development.md)**: Configuring AI coding agents (`AGENTS.md`, bundled skills, and `qmltui-runner`) to verify committed UTF-8 frames, SGR styles, and animations without a TTY.
13. **[Editor and IDE Setup](editor-setup.md)**: Configuring `qmlls`, `qmllint`, `qmlformat`, and `Runner` scenario autocomplete across VS Code, Neovim, Zed, CLion, and Qt Creator.
14. **[QmlTui in the Browser (Qt for WebAssembly)](webassembly.md)**: Compiling QmlTui applications to WebAssembly with `qmltui_wasm_app()` and embedding `WasmTerminalBackend` in `xterm.js`.

For CMake options, `find_package(QmlTui)` installation, and runtime environment variables, see [Build Configuration and Installation](../build-configuration.md).
