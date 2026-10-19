# Guides

These guides cover QmlTui's retained-mode architecture, core UI subsystems, C++20 extensibility, and developer tooling in logical reading order.

## Core Engine & UI Fundamentals

Follow these guides in order to understand how QmlTui structures, lays out, styles, and updates terminal interfaces:

1. **[Architecture](architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` cell diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and what `QmlTui.Core` (`import QmlTui`) provides standalone vs. companion modules.
2. **[Layouts & Cell Geometry](layouts.md)**: Integer cell coordinates, borders and insets, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout` engines.
3. **[Input & Focus](input-and-focus.md)**: Active focus chains, `Keys.onPressed` bubbling and interception, `Shortcut` vs. contextual `Escape` quit paths, and `MouseArea`.
4. **[Styling & Colors](styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient` / `GradientStop`, inline/reusable component styling, and `TextArea` highlight layers.
5. **[Models in QmlTui Views](models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView` with `O(viewport)` instantiation.
6. **[Animation & States](animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `Transition`, and Private Qt `Behavior` / `StateGroup`.

## Rendering Modes, C++ Extensibility & Tooling

Extend QmlTui into streaming CLIs, embedded PTY panes, custom C++20 items, deterministic headless tests, IDE language servers, and browser builds:

7. **[Inline Mode & Scrollback](inline-mode.md)**: Building streaming CLIs and coding agents with `Terminal.Inline` and `Scrollback` alongside a live bottom viewport.
8. **[Embedded PTY Shells (`QmlTui.PtyView`)](pty-view.md)**: Hosting interactive login shells and CLI processes inside `PtyView` on Linux and macOS.
9. **[Custom C++ Items](custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer` in C++, exposing QML types with `QML_ELEMENT`, or building C++-only scenes.
10. **[Testing & Recording with Runner](testing.md)**: Deterministic headless testing with a virtual clock, golden frame/style snapshots, Asciinema `.cast` recording, and `qmltui-runner`.
11. **[IDE Setup & Tooling](ide-setup.md)**: Configuring `qmlls`, `qmllint`, and `qmltui-runner.d.ts` scenario autocomplete in VS Code, Qt Creator, Neovim, and Helix.
12. **[WebAssembly Playground](webassembly.md)**: Compiling and embedding QmlTui applications in the browser with Qt for WebAssembly and `xterm.js`.
13. **[QmlTui for Qt Quick Developers](qt-quick-to-qmltui.md)**: Mapping your `QtQuick` mental model to integer cell coordinates, terminal box-drawing borders, `AltScreen` vs. `Inline` mode, and zero-`QtQuick` imports.
