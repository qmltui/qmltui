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

The experimental Unix PTY host is opt-in:

```sh
cmake -S . -B build -DQTUI_BUILD_TERMINAL_HOST=ON
cmake --build build --target qtui_terminal_view
```

This adds the `Qtui.TerminalHost` QML module and `Qtui::TerminalHost` CMake
target on Linux and macOS. See [TerminalView](docs/TerminalView.md) for its API,
security defaults, offline libvterm setup, and current compatibility limits.

View construction benchmarks are available with
`-DQTUI_BUILD_BENCHMARKS=ON`; run the resulting `benchmark_views` executable
to record 1,000- and 10,000-row creation time.

## Agent-oriented application testing

Enable `Qtui::Testing` with `QTUI_BUILD_TESTS=ON` in-tree or
`QTUI_ENABLE_TEST_RUNNER=ON` for installation. Test-enabled applications can
run scenarios through `QTUI_SCRIPT`, inline actions through `QTUI_EVAL`, or
capture one frame with `QTUI_INSPECT=1`. Runs are quiet by default and write
`result.json` plus a normalized final frame beneath
`QTUI_TEST_ARTIFACT_DIR`; set `QTUI_TEST_LIVE=1` for interactive rendering.

Installed CMake consumers can register a scenario with:

```cmake
find_package(Qtui 0.2 REQUIRED)

# Linking Qtui::Testing is what puts the runner inside the application; without
# it the scenario fails with "did not create result.json".
target_link_libraries(my_app PRIVATE Qtui::Core Qtui::Testing)

qtui_add_scenario_test(
    NAME my_app_smoke
    TARGET my_app
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
    LABELS AgentSmoke
)
```

`qtui_add_scenario_test` defaults its QML import path to
`Qtui_QML_IMPORT_PATH`, which `find_package(Qtui)` sets to the installed import
root; pass `QML_IMPORT_PATH` explicitly to override it. Link `Qtui::Testing`
only into a test-only executable — an application carrying the runner executes
the scenario named by `QTUI_SCRIPT`/`QTUI_EVAL` if those variables are set in
its environment. See `examples/apps/industrial_plc_hmi` for that split.

JavaScript scenarios can inspect cells, find rendered text, and click it
without hard-coded coordinates. See `src/Qtui/Testing/doc/TestRunner.md` for
the JavaScript and result contracts.

## Install and consume

```sh
cmake --install build --prefix /your/prefix
```

An external C++ project can use the custom-item API with:

```cmake
find_package(Qtui 0.2 REQUIRED)
target_link_libraries(my_app PRIVATE Qtui::Core)
```

Link the modules an application uses with `Qtui::Core`, `Qtui::Controls`, and
`Qtui::Extras`. For modules built in the same CMake project through
`add_subdirectory` or `FetchContent`, list the source-tree targets (`QtuiCore`,
`QtuiControls`, and `QtuiExtras`) in
`qt_add_qml_module(DEPENDENCIES TARGET ...)`. Qt does not accept imported
targets in `DEPENDENCIES TARGET`. No project-specific QML import-path variable
is required. Installed QML modules live in Qt's standard
`<prefix>/lib/qt6/qml` import root, which Qt discovers for normal Qt-prefix
installations. Applications using a non-standard prefix should expose that
standard root through Qt's usual QML import-path mechanism.

The installed QML modules are:

- `Qtui`
- `Qtui.Controls`
- `Qtui.Extras`
- `Qtui.Image`
- `Qtui.TerminalHost` when built with `QTUI_BUILD_TERMINAL_HOST=ON`

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
