# Guides

These guides cover QmlTui's retained-mode architecture, UI subsystems, C++ extensibility, and tooling.

## Core Engine and UI

1. **[Architecture](architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` cell diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and QML module boundaries.
2. **[Layouts & Cell Geometry](layouts.md)**: Integer cell coordinates, borders and insets, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout`.
3. **[Input & Focus](input-and-focus.md)**: Focus chains, `FocusScope`, `Keys.onPressed` propagation, `Shortcut`, contextual `Escape` quit handling, and `MouseArea`.
4. **[Styling & Colors](styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient` / `GradientStop`, component styling, and `TextArea` highlight layers.
5. **[Models in QmlTui Views](models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView`.
6. **[Animation & States](animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `Transition`, and optional `Behavior` / `StateGroup`.

## Rendering Modes, C++ Extensibility, and Tooling

7. **[Inline Mode & Scrollback](inline-mode.md)**: Building streaming CLIs and REPLs with `Terminal.Inline` and `Scrollback`.
8. **[Embedded PTY Shells (`QmlTui.PtyView`)](pty-view.md)**: Hosting interactive Unix child processes inside `PtyView` on Linux and macOS.
9. **[Custom C++ Items](custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer`, registering QML types with `QML_ELEMENT`, and building C++-only scenes.
10. **[Testing & Recording with Runner](testing.md)**: Headless testing with a virtual clock, text and style snapshots, Asciinema `.cast` recording, and `qmltui-runner`.
11. **[Qt Creator Setup & Tooling](qt-creator.md)**: Configuring Qt Creator with `qmltui-qtc` wizards, offline `F1` help (`qmltui.qch`), `qmlls`, `qmllint`, `Runner` actions, and live terminal previews.
12. **[WebAssembly Playground](webassembly.md)**: Building and embedding QmlTui applications in the browser with Qt for WebAssembly and `xterm.js`.
13. **[QmlTui for Qt Quick Developers](qt-quick-to-qmltui.md)**: Differences between `QtQuick` and QmlTui's character-cell coordinate system, borders, terminal modes, and imports.
