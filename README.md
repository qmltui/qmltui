# Qtui

Qtui is a retained-mode terminal UI framework for Qt 6. It exposes terminal
items, layouts, controls, views, animation, and widgets as QML modules while
using the Qt event loop and model/view APIs.

Qtui currently targets Linux and macOS terminals with UTF-8 and ANSI/VT escape
support. Windows code is experimental and compile-tested only.

## Requirements

- CMake 3.20 or newer
- Qt 6.8 with Core and Qml development packages
- A C++20 compiler

The optional `Behavior on property` feature uses a private Qt QML interface.
Enable it with `-DQTUI_ENABLE_BEHAVIOR=ON`; that configuration additionally
requires the Qt QmlPrivate development package and is tied more closely to the
Qt version used to build Qtui.

## Build

```sh
cmake -S . -B build -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

View construction benchmarks are available with
`-DQTUI_BUILD_BENCHMARKS=ON`; run the resulting `benchmark_views` executable
to record 1,000- and 10,000-row creation time.

## Install and consume

```sh
cmake --install build --prefix /your/prefix
```

An external C++ project can use the custom-item API with:

```cmake
find_package(Qtui 0.1 REQUIRED)
target_link_libraries(my_app PRIVATE Qtui::Core)
```

Link the modules an application uses with `Qtui::Core`, `Qtui::Controls`, and
`Qtui::Widgets`. For modules built in the same CMake project through
`add_subdirectory` or `FetchContent`, list the source-tree targets (`QtuiCore`,
`QtuiControls`, and `QtuiWidgets`) in
`qt_add_qml_module(DEPENDENCIES TARGET ...)`. Qt does not accept imported
targets in `DEPENDENCIES TARGET`. No project-specific QML import-path variable
is required. Installed QML modules live in Qt's standard
`<prefix>/lib/qt6/qml` import root, which Qt discovers for normal Qt-prefix
installations. Applications using a non-standard prefix should expose that
standard root through Qt's usual QML import-path mechanism.

The installed QML modules are:

- `Qtui`
- `Qtui.Controls`
- `Qtui.Widgets`

## Supported public surface

The QML modules are the primary API. The supported C++ extension surface is
the `qtui::Item` subclassing API, its rendering buffer/cell types, and key and
mouse event types needed by custom items. Renderer, terminal-device, and view
implementation details are internal and may change during the 0.x series.

Plain text renders terminal control characters visibly and never interprets
them. Escape-sequence interpretation requires an explicit ANSI text format.

See [QML API](docs/qml_api.md), [architecture](docs/architecture.md), and
[custom items](docs/custom_items.md). Breaking stabilization changes are listed
in the [migration notes](docs/migration-0.1.md).
