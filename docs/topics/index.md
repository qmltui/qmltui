# Topics & Guides

These guides cover QmlTui's declarative mental model, core engine subsystems, C++20 extensibility, and developer tooling in logical reading order.

## Mental Model & Toolkit Comparisons

Start here if you are evaluating QmlTui or mapping concepts from Qt Quick or other terminal UI libraries:

- **[Why Declarative Bindings?](why-declarative-bindings.md)**: How fine-grained Qt property bindings and a retained C++ cell tree eliminate manual dirty tracking and full-frame immediate-mode rebuilds.
- **[QmlTui for Qt Quick Developers](qt-quick-to-qmltui.md)**: Map your `QtQuick` mental model to integer cell coordinates, terminal borders, `AltScreen` vs. `Inline` mode, and zero-`QtQuick` imports.
- **[QmlTui for Ratatui, Textual, and Bubble Tea Developers](tui-comparison.md)**: Compare retained-mode QML property bindings and C++ `QAbstractItemModel` views against immediate-mode rendering, CSS-styled widgets, and Elm-style update loops.

## Core Engine & UI Fundamentals

Follow these guides in order to understand how QmlTui structures, lays out, styles, and updates terminal interfaces:

1. **[Architecture](architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` cell diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and what `QmlTui.Core` (`import QmlTui`) provides standalone vs. companion modules.
2. **[Layouts & Geometry](layouts.md)**: Integer cell coordinates, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout` engines.
3. **[Input & Focus](input-and-focus.md)**: Active focus chains, `FocusScope`, `Keys.onPressed` bubbling, `Shortcut`, SGR mouse events, and bracketed paste.
4. **[Models & Virtualized Views](models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView` with `O(viewport)` instantiation.
5. **[Styling & Colors](styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient`, `Border` styles, and application-owned QML component styling.
6. **[Animation & States](animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `Transition`, and optional Private Qt `Behavior`.

## Rendering Modes, C++ Extensibility & Tooling

Extend QmlTui beyond full-screen QML dashboards into streaming CLIs, custom C++20 items, IDE language servers, and browser builds:

- **[Inline / Stream Mode](inline-mode.md)**: Building streaming CLIs and coding agents with `Terminal.Inline` and `Scrollback` alongside a live bottom viewport.
- **[Custom C++ Items](custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer` in C++, exposing QML types with `QML_ELEMENT`, or building C++-only scenes.
- **[IDE Setup](ide-setup.md)**: Configuring `qmlls` and `qmllint` in Qt Creator, VS Code, Neovim, CLion, and Helix for full autocomplete and static type checking.
- **[WebAssembly Playground](webassembly.md)**: Compiling and embedding QmlTui applications in the browser.
