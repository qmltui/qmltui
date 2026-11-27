# Guides

These guides cover QmlTui's retained-mode cell architecture, UI subsystems, C++ extensibility, and testing tools.

## Core UI and Architecture

1. **[Architecture](architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` cell diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and QML module boundaries.
2. **[Migrating from Qt Quick](migrating-from-qt-quick.md)**: Imports, integer cell geometry and border padding, `Text.AutoText`, `Keys.BeforeItem`, fixed-stride views, `Loader`, and styling differences for Qt Quick developers.
3. **[Layouts & Cell Geometry](layouts.md)**: Integer cell coordinates, borders and insets, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout`.
4. **[Input & Focus](input-and-focus.md)**: Focus chains, `FocusScope`, `Keys.onPressed` propagation, `Shortcut`, contextual `Escape` quit handling, and `MouseArea`.
5. **[Styling & Colors](styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient` / `GradientStop`, component styling, and `TextArea` highlight layers.
6. **[Models in QmlTui Views](models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView`.
7. **[Python and PySide6](python-pyside6.md)**: Building public-Qt QML plugins and hosting `QmlTui` and `QmlTui.Controls` from Python with `QAbstractTableModel` and `QSortFilterProxyModel`.
8. **[Animation & States](animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `StateGroup`, `Transition`, and optional `Behavior`.

## Rendering Modes, C++ Extensibility, and Tooling

9. **[Inline Mode & Scrollback](inline-mode.md)**: Building streaming CLIs and REPLs with `Terminal.Inline` and `Scrollback`.
10. **[Custom C++ Items](custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer`, registering QML types with `QML_ELEMENT`, and building C++-only scenes.
11. **[Testing & Recording with Runner](testing.md)**: Headless testing with a virtual clock, text and style snapshots, Asciinema `.cast` recording, and `qmltui-runner`.
12. **[Agentic Development Loop with Runner](agentic-development.md)**: Headless closed-loop verification for coding agents using coordinate-ruled multi-viewport sweeps, SGR style inspection, virtual-clock stepping, idle paint budgets, `.cast` recordings, and bundled skills.
13. **[Editor and IDE Setup](editor-setup.md)**: Configuring `qmlls`, `qmllint`, `qmlformat`, and `Runner` scenario autocomplete across VS Code, Neovim, Zed, CLion, and Qt Creator.
14. **[QmlTui in the Browser (Qt for WebAssembly)](webassembly.md)**: Compiling QmlTui applications to WebAssembly with `qmltui_wasm_app()` and embedding `WasmTerminalBackend` in `xterm.js`.

For CMake options, `find_package(QmlTui)` installation, and runtime environment variables, see [Build Configuration and Installation](../build-configuration.md). For framework contribution checklists (adding QML types, recipes, sanitizers, and coverage), see [Adding Features](../contributing/adding-features.md).
