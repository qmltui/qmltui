# Getting started

## Requirements

- CMake 3.20 or newer
- Qt 6.8 with Core and Qml development packages
- A C++20 compiler

The optional `Qamui.Image` module requires Qt Gui and Network development packages.
It is enabled by default (`-DQAMUI_ENABLE_IMAGE=ON`); disable it with `-DQAMUI_ENABLE_IMAGE=OFF`
for a minimal terminal-only setup without GUI or network dependencies.

The optional `Behavior on property` feature uses a private Qt QML interface.
Enable it with `-DQAMUI_ENABLE_BEHAVIOR=ON`; that configuration additionally
requires the Qt QmlPrivate development package and is tied more closely to the
Qt version used to build Qamui.


## Build

```sh
cmake -S . -B build -DQAMUI_BUILD_TESTS=ON -DQAMUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The experimental Unix PTY host is opt-in:

```sh
cmake -S . -B build -DQAMUI_ENABLE_TERMINAL_HOST=ON
cmake --build build --target qamui_terminal_view_demo
```

This adds the `Qamui.TerminalHost` QML module and `Qamui::TerminalHost` CMake
target on Linux and macOS. See [TerminalView](qml-types/terminalview.md) for its API,
security defaults, offline libvterm setup, and current compatibility limits.

The examples keep inline agent output and hosted shells separate:

- `qamui_agent_stream_demo` (`examples/topics/agent_stream`) demonstrates an
  inline agent session with scrollback, prompts, and permission decisions. It
  does not require TerminalHost.
- `qamui_terminal_view_demo` (`examples/components/terminal_view`) demonstrates two
  interactive shells and a command dispatcher in full-screen mode. It requires
  `QAMUI_ENABLE_TERMINAL_HOST=ON`; press Ctrl+Q to quit.

Benchmarks are available with `-DQAMUI_BUILD_BENCHMARKS=ON`: `benchmark_views`
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
find_package(Qamui 0.5 REQUIRED)
target_link_libraries(my_app PRIVATE Qamui::Core)
```

Link the modules an application uses with `Qamui::Core`, `Qamui::Controls`, and
`Qamui::Image`. For modules built in the same CMake project through
`add_subdirectory` or `FetchContent`, list the source-tree targets (`QamuiCore`,
and `QamuiControls`) in
`qt_add_qml_module(DEPENDENCIES TARGET ...)`. Qt does not accept imported
targets in `DEPENDENCIES TARGET`. No project-specific QML import-path variable
is required. Installed QML modules live in Qt's standard
`<prefix>/lib/qt6/qml` import root, which Qt discovers for normal Qt-prefix
installations. Applications using a non-standard prefix should expose that
standard root through Qt's usual QML import-path mechanism.

The installed QML modules are:

- `Qamui`
- `Qamui.Controls`
- `Qamui.Image`
- `Qamui.TerminalHost` when built with `QAMUI_ENABLE_TERMINAL_HOST=ON`


## Next Steps

- Configure your development environment with the [IDE setup guide](topics/ide-setup.md).
- Coming from Qt Quick? Read [Qamui for Qt Quick developers](topics/qt-quick-to-qamui.md).
- Coming from Ratatui, Bubble Tea, or Textual? Read [Qamui for TUI developers](topics/tui-comparison.md).
- Follow the hands-on [first tutorial](tutorials/01-first-terminal.md).
