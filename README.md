<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/branding/qamui_lockup_dark.svg">
    <img alt="Qamui" src="docs/assets/branding/qamui_lockup_light.svg" width="360">
  </picture>
</p>

Qamui is a retained-mode terminal UI framework for Qt 6. It exposes terminal
items, layouts, controls, views, animation, and widgets as QML modules while
using the Qt event loop and model/view APIs.

Qamui targets Linux and macOS terminals and Windows Terminal, all with UTF-8 and
ANSI/VT escape support. Terminal I/O goes through one platform backend per OS
(`termios` and `SIGWINCH` on POSIX, virtual-terminal console modes on Windows);
everything above it is shared. The optional PTY-backed `TerminalView` module is
Unix only.

## QML at a glance

The UI stays declarative while Qamui handles terminal rendering, focus, input,
and model/view interaction:

```qml
import QtQml
import Qamui

Terminal {
    id: root
    color: "#0d1117"

    property string message: qsTr("↑/↓ navigate • Enter select • Q quit")

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text { text: qsTr("⚡ SERVICE MONITOR"); color: "cyan"; bold: true }

        TableView {
            focus: true
            width: parent.width
            height: parent.height - 4
            alternatingRows: true
            showColumnGrid: true

            columns: [
                { title: qsTr("SERVICE"), width: 24 },
                { title: qsTr("LATENCY"), width: 12, align: "right" },
                { title: qsTr("STATUS"), width: 16, align: "center" }
            ]

            model: [
                ["api-gateway", "12 ms", "healthy"],
                ["postgres", "8 ms", "healthy"],
                ["worker-01", "86 ms", "degraded"]
            ]

            cellFormat: (row, column, value) => {
                if (column !== 2)
                    return { text: value }
                const healthy = value === "healthy"
                return {
                    text: healthy ? qsTr("● HEALTHY") : qsTr("▲ DEGRADED"),
                    color: healthy ? "green" : "yellow",
                    bold: true
                }
            }

            onRowActivated: (row) => {
                root.message = qsTr("Selected service row %1").arg(row + 1)
            }
        }

        Text { text: root.message; color: "gray" }
    }
}
```

The complete runnable application and its rendered-frame smoke test are in
[`examples/apps/service_monitor`](examples/apps/service_monitor).

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
cmake --build build --target qamui_terminal_view
```

This adds the `Qamui.TerminalHost` QML module and `Qamui::TerminalHost` CMake
target on Linux and macOS. See [TerminalView](docs/TerminalView.md) for its API,
security defaults, offline libvterm setup, and current compatibility limits.

The examples keep inline agent output and hosted shells separate:

- `qamui_agent_stream_demo` (`examples/concepts/agent_stream`) demonstrates an
  inline agent session with scrollback, prompts, and permission decisions. It
  does not require TerminalHost.
- `qamui_terminal_view` (`examples/components/terminal_view`) demonstrates two
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
See the [0.4 acceptance report](docs/benchmarks/0.4-acceptance/README.md) for
the comparison method, results, and validation limits.

## Terminal capabilities

Qamui detects the terminal's colour depth once at startup: `NO_COLOR` or
`TERM=dumb` disable colour, `COLORTERM=truecolor`/`24bit` or a known
`TERM_PROGRAM` select 24-bit colour, a `TERM` containing `256color` without
`COLORTERM` selects the 256-colour palette, and anything else keeps 24-bit
output. Override with `QAMUI_COLOR_DEPTH=mono|16|256|truecolor`. Frames are
bracketed with synchronized-output sequences on terminals; set
`QAMUI_SYNC_OUTPUT=0` to disable that.

Rendering is paced to at most 60 frames per second (`QAMUI_MAX_FPS`, 0 for
uncapped) and terminal output is non-blocking: bytes a slow terminal cannot
accept yet are queued and drained from the event loop, and new frames wait
while more than `QAMUI_OUTPUT_QUEUE_LIMIT` bytes (default 262144) are queued.
`QAMUI_FRAME_STATS=1` logs paint, diff and write cost per frame.

## Autonomous agent harness and application scenarios

Enable `Qamui::Runner` with `QAMUI_BUILD_TESTS=ON` in-tree or
`QAMUI_ENABLE_RUNNER=ON` for installation. Runner-enabled applications can
run scenarios through `QAMUI_RUNNER_SCRIPT`, inline actions through `QAMUI_RUNNER_EVAL`, or
capture one frame with `QAMUI_RUNNER_CAPTURE_AFTER_MS=<delay>`. Runs are quiet by default and write
`result.json` plus a normalized final frame beneath
`QAMUI_RUNNER_ARTIFACT_DIR`; set `QAMUI_RUNNER_LIVE=1` for interactive rendering.
A harness that constructs `qamui::Runner` itself uses `Runner::useEmbeddedScenario()`,
`setLiveOutput()`, and `setUpdateSnapshots()` instead of those variables.

Installed CMake consumers can register a scenario with:

```cmake
find_package(Qamui 0.5 REQUIRED)

# Linking Qamui::Runner is what puts the runner inside the application; without
# it the scenario fails with "did not create result.json".
target_link_libraries(my_app PRIVATE Qamui::Core Qamui::Runner)

qamui_add_scenario(
    NAME my_app_smoke
    TARGET my_app
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
    LABELS AgentSmoke
)
```

`qamui_add_scenario` defaults its QML import path to
`Qamui_QML_IMPORT_PATH`, which `find_package(Qamui)` sets to the installed import
root; pass `QML_IMPORT_PATH` explicitly to override it. Link `Qamui::Runner`
only into a dedicated test or harness executable — an application carrying the runner executes
the scenario named by `QAMUI_RUNNER_SCRIPT`/`QAMUI_RUNNER_EVAL` if those variables are set in
its environment. See `examples/apps/industrial_plc_hmi` for that split.

JavaScript scenarios can inspect cells, find rendered text, click it without
hard-coded coordinates, and compare styled snapshots that cover colors and
attributes as well as text. See `src/Qamui/Runner/doc/Runner.md` for
the JavaScript and result contracts.

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

## Supported public surface

The QML modules are the primary API. The supported C++ extension surface is
the `qamui::Item` subclassing API, its rendering buffer/cell types, key and
mouse event types needed by custom items, and the `qamui::TerminalBackend`
factory for applications that talk to something other than the process's own
terminal. Renderer, terminal-device, and view implementation details are
internal and may change during the 0.x series. Core items and Controls use terminal-native foreground and background colors.
Applications customize them through ordinary color properties and reusable QML components.

Plain text renders terminal control characters visibly and never interprets
them. Escape-sequence interpretation requires an explicit ANSI text format.

See [QML API](docs/qml_api.md), [models in views](docs/guide/models.md),
[Qamui in the browser](docs/guide/webassembly.md),
[architecture](docs/architecture.md),
[why declarative bindings](docs/guide/why-declarative-bindings.md), and
[custom items](docs/custom_items.md). Breaking stabilization changes are listed
in the [0.3 migration notes](docs/migration/0.3.md).

## License

Qamui is available under the LGPLv3 and GPLv3, or under a commercial license
for teams that cannot meet those terms (static linking on devices, closed
distribution, WebAssembly). Examples and the starter template are
BSD-3-Clause; documentation is CC-BY-4.0. The license of a file follows from
its path, see [LICENSE.md](LICENSE.md). Commercial licensing: contact the
copyright holder through the project page.

Qamui is an independent project and not a product of The Qt Company.
