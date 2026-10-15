# QmlTui Documentation

**QmlTui** is a C++20 and Qt 6.8+ retained-mode terminal UI framework. Build full-screen terminal applications or inline streaming CLIs using `QtQml` and `QmlTui` modules, integer cell geometry, reactive property bindings, `QAbstractItemModel` views, and deterministic headless testing.

---

## Start Here

New to QmlTui? Follow these guides to set up your environment, run the interactive gallery, and build your first application:

- **[Getting Started](getting-started.md)**: System requirements, running `qmltui_gallery`, creating a standalone 3-file application (`FetchContent` or `find_package`), CMake options, and environment variables.
- **[Six-Part Tutorial](tutorials/index.md)**: Hands-on walkthrough covering `Terminal`, layouts and rich text, focus and keyboard input, models and views, controls and styling, and automated `Runner` testing.
- **[Examples & Component Cookbook](examples.md)**: Understand what lives in `QmlTui.Controls` versus copyable QML recipes, and browse every runnable component and topic demo.
- **[IDE Setup](topics/ide-setup.md)**: Configure `qmlls` and `qmllint` in Qt Creator, VS Code, Neovim, CLion, and Helix for full autocomplete and type checking.

## Coming From Another Toolkit?

Already familiar with Qt Quick or terminal UI libraries in Rust, Python, or Go? Start with the guide tailored to your background:

- **[QmlTui for Qt Quick Developers](topics/qt-quick-to-qmltui.md)**: Map your `QtQuick` mental model to integer cell coordinates, terminal borders, `Frame` vs. `Stream` mode, and zero-`QtQuick` imports.
- **[QmlTui for Ratatui, Textual, and Bubble Tea Developers](topics/tui-comparison.md)**: Compare retained-mode QML property bindings and `QAbstractItemModel` views against immediate-mode rendering, CSS-styled widgets, and Elm-style update loops.
- **[Why Declarative Bindings?](topics/why-declarative-bindings.md)**: How fine-grained property dependencies and retained cell trees eliminate manual dirty tracking and boilerplate state synchronization.

## Core Guides

Deep-dive into QmlTui's subsystems and architectural patterns (or view the [Topics index](topics/index.md)):

- **[Architecture](topics/architecture.md)**: Retained item tree, double-buffered `ScreenBuffer` diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and optional modules.
- **[Layouts & Geometry](topics/layouts.md)**: Integer cell coordinates, `anchors`, `Row`/`Column`/`Grid` positioners, `RowLayout`/`ColumnLayout`/`GridLayout` engines, and `SplitView`.
- **[Input & Focus](topics/input-and-focus.md)**: Active focus chains, `FocusScope`, `KeyNavigation`, `Keys.onPressed` bubbling, `Shortcut`, SGR mouse events, and bracketed paste.
- **[Models & Virtualized Views](topics/models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, `GridView`, and `LogView`.
- **[Styling & Colors](topics/styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient`, `Border` styles, and application-owned QML component styling.
- **[Animation & States](topics/animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `SequentialAnimation`, `ParallelAnimation`, `State`, `Transition`, and optional Private Qt `Behavior`.
- **[Inline / Stream Mode](topics/inline-mode.md)**: Building streaming CLIs and coding agents with `Terminal.Stream` and `Scrollback` alongside a live bottom viewport.
- **[Custom C++ Items](topics/custom-items.md)**: Subclassing `qmltui::Item` and `paint(Painter *)` in C++, exposing QML types with `QML_ELEMENT`, or building C++-only scenes.
- **[WebAssembly Playground](topics/webassembly.md)**: Compiling and embedding QmlTui applications in the browser.

## API Reference

- **[QML Types Reference](qml-types/index.md)**: Complete property, method, and signal documentation for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.TerminalHost`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Item`, `Terminal`, `Painter`, `ScreenBuffer`, `Style`, `Color`, `TerminalCapabilities`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history, new features, and migration guides.

## Contributing & Internals

- **[CI Runners](contributing/ci-runners.md)**: Continuous integration matrix and headless test execution.
- **[Recording Demos](contributing/recording.md)**: Capturing deterministic `.cast` recordings and GIF assets with `Runner`.
- **[Development Plans](contributing/plans/README.md)**: Design documents and milestone architecture records.
