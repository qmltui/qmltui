# Getting started

## Requirements

- CMake 3.20 or newer
- Qt 6.8 with Core and Qml development packages
- A C++20 compiler

The optional `QmlTui.Image` module requires Qt Gui and Network development packages.
It is enabled by default (`-DQMLTUI_ENABLE_IMAGE=ON`); disable it with `-DQMLTUI_ENABLE_IMAGE=OFF`
for a minimal terminal-only setup without GUI or network dependencies.

The optional `Behavior on property`, `StateGroup`, and advanced states features
use private Qt QML interfaces. Enable them with `-DQMLTUI_ENABLE_PRIVATE_QT=ON`;
that configuration additionally requires the Qt `QmlPrivate` development package
and is tied more closely to the Qt version used to build QmlTui.


## Build

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The experimental Unix PTY host is opt-in:

```sh
cmake -S . -B build -DQMLTUI_ENABLE_TERMINAL_HOST=ON
cmake --build build --target qmltui_terminal_view_demo
```

This adds the `QmlTui.TerminalHost` QML module and `qmltui::terminal_host` CMake
target on Linux and macOS. See [TerminalView](qml-types/terminalview.md) for its API,
security defaults, offline libvterm setup, and current compatibility limits.

The examples keep inline agent output and hosted shells separate:

- `qmltui_agent_stream_demo` (`examples/topics/agent_stream`) demonstrates an
  inline agent session with scrollback, prompts, and permission decisions. It
  does not require TerminalHost.
- `qmltui_terminal_view_demo` (`examples/components/terminal_view`) demonstrates two
  interactive shells and a command dispatcher in full-screen mode. It requires
  `QMLTUI_ENABLE_TERMINAL_HOST=ON`; press Ctrl+Q to quit.

Benchmarks are available with `-DQMLTUI_BUILD_BENCHMARKS=ON`: `benchmark_views`
records 1,000- and 10,000-row view creation time, and `benchmark_render`
records per-frame paint and diff cost for representative scenes at 80x24 and
240x70 (`--json <path>` for trending). Run `benchmark_render` before and after
rendering changes and record the numbers in the pull request.
`--frames <count>` changes the measured samples per case (default 200, after
5 warm-up frames). JSON also includes setter time (`updateMedianUs`) and the
median combined setter/paint cost (`updatePaintMedianUs`), so text-cache rebuilds
are accounted for in changing-text workloads. These measurements exclude
terminal writes and terminal-emulator drawing; an idle benchmark deliberately
requests unchanged frames, whereas an idle application need not render at all.
See the [0.4 acceptance report](contributing/benchmarks/0.4-acceptance/README.md) for
the comparison method, results, and validation limits.


## Install and consume

```sh
cmake --install build --prefix /your/prefix
```

An external C++ project can use the custom-item API with:

```cmake
find_package(QmlTui 0.5 REQUIRED)
target_link_libraries(my_app PRIVATE qmltui::core)
```

Link the modules an application uses with `qmltui::core`, `qmltui::controls`, and
`qmltui::image`.

- **In-tree (`add_subdirectory` or `FetchContent`)**: List the target aliases
  (`DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls`) in
  `qt_add_qml_module(...)` so CMake orders QML module generation and `qmllint`
  finds the build-tree modules under `QT_QML_OUTPUT_DIRECTORY`.
- **Installed (`find_package(QmlTui)`)**: Qt's `qt_add_qml_module(DEPENDENCIES TARGET ...)`
  does not accept `IMPORTED` targets. Instead, link `qmltui::core` / `qmltui::controls`
  with `target_link_libraries()` and use the exported `QmlTui_QML_IMPORT_PATH`
  variable when configuring QML tooling or runtime import paths. Installed QML
  modules live under `<prefix>/lib/qml` by default (configurable via
  `QMLTUI_INSTALL_QMLDIR`).

The installed QML modules are:

- `QmlTui`
- `QmlTui.Controls`
- `QmlTui.Image`
- `QmlTui.TerminalHost` when built with `QMLTUI_ENABLE_TERMINAL_HOST=ON`


## CMake options and environment variables

### Top-level CMake options

| Option | Default | Description |
| :--- | :--- | :--- |
| `QMLTUI_BUILD_EXAMPLES` | `ON` (top-level) | Build example and tutorial executables |
| `QMLTUI_BUILD_TESTS` | `ON` (top-level) | Build unit tests and enable Runner scenarios |
| `QMLTUI_BUILD_BENCHMARKS` | `ON` (top-level) | Build performance benchmark executables |
| `QMLTUI_ENABLE_PRIVATE_QT` | `OFF` | Enable `Behavior`, `StateGroup`, and advanced states (`Qt6::QmlPrivate`) |
| `QMLTUI_ENABLE_IMAGE` | `ON` | Build the `QmlTui.Image` module (`Qt6::Gui`, `Qt6::Network`) |
| `QMLTUI_ENABLE_TERMINAL_HOST` | `OFF` | Build the Unix PTY `QmlTui.TerminalHost` module (Linux/macOS) |
| `QMLTUI_ENABLE_RUNNER` | `OFF` | Build and install `qmltui::runner` and the `qmltui-runner` CLI |
| `QMLTUI_INSTALL_QMLDIR` | `${CMAKE_INSTALL_LIBDIR}/qml` | Install directory for QmlTui QML modules (`<prefix>/lib/qml`) |
| `QMLTUI_USE_SYSTEM_UTF8PROC` | `OFF` | Link system `utf8proc` instead of the bundled copy |
| `QMLTUI_USE_SYSTEM_LIBVTERM` | `OFF` | Link system `libvterm` instead of the fetched source (`TerminalHost` only) |
| `QMLTUI_COMMERCIAL_LICENSE` | `OFF` | Acknowledge building under a commercial QmlTui license |

### Runtime environment variables

| Variable | Values / Units | Description |
| :--- | :--- | :--- |
| `NO_COLOR` | non-empty | Disable ANSI color output (`ColorDepth::Monochrome`) |
| `QMLTUI_COLOR_DEPTH` | `mono`/`0`, `16`, `256`, `truecolor`/`24bit` | Override auto-detected terminal color depth |
| `QMLTUI_MAX_FPS` | integer `1`–`240` | Override maximum render frame rate |
| `QMLTUI_FRAME_STATS` | `1` / non-empty | Log periodic render timing and byte statistics to stderr |
| `QMLTUI_OUTPUT_QUEUE_LIMIT` | bytes (`>= 1024`) | Maximum queued async terminal output before backpressure drops frames |
| `QMLTUI_SYNC_OUTPUT` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) DEC mode 2026 synchronized output |
| `QMLTUI_KITTY_KEYBOARD` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) Kitty keyboard protocol negotiation |
| `QMLTUI_KITTY_GRAPHICS` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) Kitty graphics protocol detection |


## Next Steps

- Configure your development environment with the [IDE setup guide](topics/ide-setup.md).
- Coming from Qt Quick? Read [QmlTui for Qt Quick developers](topics/qt-quick-to-qmltui.md).
- Coming from Ratatui, Bubble Tea, or Textual? Read [QmlTui for TUI developers](topics/tui-comparison.md).
- Follow the hands-on [first tutorial](tutorials/01-first-terminal.md).
