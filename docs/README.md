# QmlTui Documentation

**QmlTui** is a retained-mode terminal UI framework for Qt 6.8+ and C++17. Declare layouts, box-drawing borders, focus chains, states, and property bindings in QML while keeping models and business logic in C++ (`QCoreApplication`, `QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`), without requiring `QtQuick`, `QtGui`, or a display server.

## Interactive Component Gallery

Browse 45 interactive controls, views, layouts, animations, and terminal capabilities in `qmltui_gallery`:

![QmlTui Interactive Component Gallery](assets/casts/gallery-tour.gif)

## Start Here

- **Build your first terminal app**: Follow the [Getting Started Tutorial](getting-started.md) to create a standalone 3-file CMake project, bind a C++ `QObject` with `QML_ELEMENT`, and run a headless `Runner` test in CTest (or copy [`examples/helloworld/`](../examples/helloworld/)).
- **Coming from Qt Quick or Qt Widgets**: Read [QmlTui for Qt Quick and Widgets Developers](guides/qt-quick-to-qmltui.md) and [Models in QmlTui Views](guides/models.md) for imports, integer cell coordinates, borders, colors, keyboard quit paths, and shared C++ models.
- **Configure CMake, editors, CI tests, and AI agents**: Set up `FetchContent` or `find_package(QmlTui)` in [Build Configuration and Installation](build-configuration.md), enable `qmlls` and `Runner` autocomplete in [Editor and IDE Setup](guides/editor-setup.md), write snapshot tests in [Testing and Recording with Runner](guides/testing.md), and configure closed-loop verification for coding agents in [Agentic Development Loop with Runner](guides/agentic-development.md).
- **Explore examples and full-scale applications**: Browse local build commands and focused recipes in [Examples and Recipes](examples.md) and 12 complete showcase applications (`qtop`, `agent_stream`, `file_manager`, `package_manager`, `audio_player`, and more) in [qmltui/apps](https://github.com/qmltui/apps).

## Guides and API Reference

- **[Guides Index](guides/index.md)**: Architecture, layouts, input and focus, styling, models, animation and states, inline mode, `PtyView`, custom C++ items, `Runner` testing, agentic workflows, editor setup, and WebAssembly.
- **[QML Types Reference](qml-types/index.md)**: Properties, methods, and signals for all 75 types across `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Terminal`, `Item`, `TextGridBuffer`, `Cell`, `Color`, `TerminalCapabilities`, `PtyView`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history and migration notes.
- **[Contributing](../CONTRIBUTING.md)**: Local development workflow, [CI Runners](contributing/ci-runners.md), and [Recording Demos](contributing/recording.md).
