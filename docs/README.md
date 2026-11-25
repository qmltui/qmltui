# QmlTui Documentation

**QmlTui** is a retained-mode terminal UI framework for Qt 6.8+ and C++17. Declare layouts, box-drawing borders, focus chains, states, and property bindings in QML while keeping models and business logic in C++ (`QCoreApplication`, `QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`), without requiring `QtQuick`, `QtGui`, or a display server.

## Interactive Component Gallery

Browse 42 default interactive demos (45 with optional `QmlTui.Image` and `QmlTui.PtyView` enabled) covering all 75 built-in types in `qmltui_gallery`:

![QmlTui Interactive Component Gallery](assets/casts/gallery-tour.gif)

## Start Here

- **Build your first terminal app**: Follow the [Getting Started Tutorial](getting-started.md) (including a 3-minute Qt/QML primer for newcomers) to create a standalone 3-file CMake project, bind a C++ `QObject` with `QML_ELEMENT`, and run a headless `Runner` test in CTest (or copy [`examples/helloworld/`](../examples/helloworld/)). Coming from Qt Quick? Start with [Migrating from Qt Quick](guides/migrating-from-qt-quick.md).
- **Learn the core UI model**: Read [Layouts & Cell Geometry](guides/layouts.md), [Input & Focus](guides/input-and-focus.md), [Styling & Colors](guides/styling-and-colors.md), [Models in QmlTui Views](guides/models.md), and [Python and PySide6](guides/python-pyside6.md) for integer cell coordinates, borders, keyboard quit paths, terminal colors, and shared C++/Python models.
- **Configure CMake, editors, CI tests, and AI agents**: Set up `FetchContent` or `find_package(QmlTui)` in [Build Configuration and Installation](build-configuration.md), enable `qmlls` and `Runner` autocomplete in [Editor and IDE Setup](guides/editor-setup.md), write snapshot tests in [Testing and Recording with Runner](guides/testing.md), and configure closed-loop verification for coding agents in [Agentic Development Loop with Runner](guides/agentic-development.md).
- **Explore examples, official add-ons, and full-scale applications**: Browse local build commands, focused recipes, and standalone official add-on modules ([`QmlTui.TVision`](https://github.com/qmltui/tvision) and [`QmlTui.Markdown`](https://github.com/qmltui/markdown)) in [Examples and Recipes](examples.md#official-add-on-modules), plus 13 complete showcase applications (`qtop`, `agent_stream`, `file_manager`, `package_manager`, `turbo_hex`, `markdown_viewer`, `audio_player`, and more) in [qmltui/apps](https://github.com/qmltui/apps).

## Guides and API Reference

- **[Guides Index](guides/index.md)**: Architecture, Qt Quick migration, layouts, input and focus, styling, models, Python (PySide6), animation and states, inline mode, `PtyView`, custom C++ items, `Runner` testing, agentic workflows, editor setup, and WebAssembly.
- **[QML Types Reference](qml-types/index.md)**: Properties, methods, and signals for all 75 types across `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Terminal`, `Item`, `TextGridBuffer`, `Cell`, `Color`, `TerminalCapabilities`, `PtyView`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history and migration notes.
- **[Contributing](../CONTRIBUTING.md)**: Local development workflow, [Adding Features](contributing/adding-features.md), [CI Runners](contributing/ci-runners.md), and [Recording Demos](contributing/recording.md).
