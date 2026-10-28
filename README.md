<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/branding/qmltui_lockup_dark.svg">
    <img alt="QmlTui" src="docs/assets/branding/qmltui_lockup_light.svg" width="360">
  </picture>
</p>

<p align="center">
  <a href="https://github.com/qmltui-dev/qmltui/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/qmltui-dev/qmltui/actions/workflows/ci.yml/badge.svg?branch=main"></a>
  <a href="https://github.com/qmltui-dev/qmltui/blob/badges/summary.md"><img alt="Coverage" src="https://github.com/qmltui-dev/qmltui/raw/badges/coverage.svg"></a>
</p>

**QmlTui** is a retained-mode terminal UI framework for Qt 6.8+ and C++17. Think of QmlTui as the **Qt Quick equivalent for terminal user interfaces**: you express the UI in declarative, reactive QML (layouts, property bindings, states, and reusable components) and build the backend in C++ (`QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`). `QmlTui` renders the QML item tree into a double-buffered character-cell grid (`TextGridBuffer`), diffs consecutive frames, and writes only the changed cells and SGR attributes to the terminal. Core applications run on `QCoreApplication` with `Qt6::Core` and `Qt6::Qml`, without requiring `QtQuick`, `QtGui`, or a display server.

## When to Use QmlTui

Use QmlTui when you want to:

- **Build terminal UIs with Qt 6 and QML**: Declare layouts, box-drawing borders, focus chains, states, and animations in QML instead of writing manual redraw loops.
- **Build streaming CLIs, REPLs, and embedded shells**: Keep a live reactive prompt at the bottom of the normal screen buffer while committing rich QML delegates into native terminal scrollback (`Terminal.Inline` and `Scrollback`), or embed interactive Unix subshells (`QmlTui.PtyView`).
- **Reuse C++ models and business logic**: Bind existing `QObject` controllers, `QAbstractItemModel` subclasses, and `QSortFilterProxyModel` pipelines to virtualized `ListView`, `TableView`, `TreeView`, and `GridView` components.
- **Ship headless or SSH tools alongside a Qt GUI**: Share the same C++ backend between a desktop `QtQuick` or `QtWidgets` application and a terminal diagnostic, installer, or industrial HMI binary.
- **Test terminal frames in CI without a live TTY**: Drive keyboard and mouse events deterministically with `qmltui::Runner`, step a virtual clock, assert committed text and style snapshots in CTest, and record Asciinema v2 `.cast` files.

Do not use QmlTui when:

- **You need pixel geometry or a GPU scene graph**: QmlTui lays out items on an integer character-cell grid (columns and rows). Use `QtQuick` or `QtWidgets` for pixel-addressed desktop and embedded displays.
- **Your project does not use C++ or Qt 6**: QmlTui requires CMake 3.21+, a C++17 compiler, and Qt 6.8+.
- **Your target is legacy `conhost.exe` on Windows**: Runtime targets are Linux, macOS, and Windows Terminal (`Win32ConsoleBackend` over Virtual Terminal sequences).

## Hello, World!

A minimal application using `import QtQml` and `import QmlTui` (from [`examples/helloworld/`](examples/helloworld/)):

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    property int count: 0

    title: count === 0 ? "Hello QmlTui!" : "Interactions: " + root.count
    color: "#0d1117"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            console.log("Goodbye!");
            Qt.quit();
        } else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            root.count++;
        } else if (event.key === Qt.Key_R) {
            root.count = 0;
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 52
        height: 11
        border {
            style: Border.Single
            color: "#2cde85"
        }

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                text: "Hello, QmlTui!"
                color: "#00ff66"
                bold: true
            }

            Text {
                text: "QML Declarative terminal UIs"
                color: "#2cde85"
                dim: true
            }

            Text {
                text: "Interactions: " + root.count
                color: "#c9d1d9"
            }

            Text {
                text: "[Enter] Count  •  [R] Reset  •  [Q] Quit"
                color: "#8b949e"
                dim: true
            }
        }
    }
}
```

Rendered in the terminal:

![Hello, QmlTui! rendered in the terminal](docs/assets/casts/hello-world.gif)

## Quickstart

### Prerequisites

- **CMake 3.21** or newer (`3.20+` when building QmlTui in-tree)
- **C++17** compiler
- **Qt 6.8 or newer** (`Core` and `Qml`, plus `Gui` and `Network` when `QMLTUI_ENABLE_IMAGE=ON`)

### Build and Run the Component Gallery

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

You can also try every gallery component in the browser with the [WebAssembly Playground](docs/playground.md) (see the [WebAssembly build guide](docs/guides/webassembly.md)).

![QmlTui Interactive Component Gallery](docs/assets/casts/gallery-tour.gif)

### Create a Standalone Application

Follow the [Getting Started tutorial](docs/getting-started.md) to build a 3-file project (`CMakeLists.txt`, `main.cpp`, `Main.qml`) and bind a C++ `QObject` with `QML_ELEMENT`, or copy [`examples/helloworld/`](examples/helloworld/) directly.

## QML Modules and CMake Targets

| QML Import | CMake Target | Required Dependencies | Provided Functionality |
| :--- | :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | `Qt6::Core`, `Qt6::Qml`, `utf8proc` (fetched by default) | `Terminal` (`AltScreen` and `Inline` modes), `Scrollback`, `Item`, `FocusScope`, `Popup`, `Rectangle`, `Border`, `Text`, `TextInput`, `TextArea`, layouts, virtualized data views (`ListView`, `TableView`, `TreeView`, `GridView`, `ScrollView`), `Canvas`, input, states, and animations |
| `import QmlTui.Controls` | `qmltui::controls` | `qmltui::core` | Interactive controls (`AbstractButton`, `Button`, `ButtonGroup`, `CheckBox`, `RadioButton`, `ComboBox`, `SpinBox`, `Slider`, `ProgressBar`, `BusyIndicator`, `Sparkline`, `TabBar`, `StackView`, `Panel`, `Dialog`, `ToolTip`) |
| `import QmlTui.Image` | `qmltui::image` | `qmltui::core`, `Qt6::Gui`, `Qt6::Network` | Offscreen raster decoding (`QImage`) and terminal graphics (`Image`, `AnimatedSprite`) using Kitty, Sixel, or Unicode half-block fallback (`-DQMLTUI_ENABLE_IMAGE=ON`; no display server required) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | `qmltui::core`, `libvterm` (fetched by default) | Embedded Unix pseudo-terminal (`PtyView`) on Linux and macOS (`-DQMLTUI_ENABLE_PTYVIEW=ON`) |

See [Build Configuration and Installation](docs/build-configuration.md) for `FetchContent`, `find_package(QmlTui)`, CMake options, and runtime environment variables.

## Performance and Memory Characteristics

- **24-byte value-type `Cell`**: [`Cell`](docs/cpp-classes/cell.md) is 24 bytes and trivially copyable. UTF-8 grapheme clusters up to 11 bytes (single code points, emoji with skin tones or variation selectors, flags, and keycaps) are stored inline without per-cell heap allocation. Consecutive `TextGridBuffer` frames are diffed cell by cell and emitted inside DEC mode 2026 synchronized output blocks.
- **Sub-millisecond view and frame latencies**: Against a 16,667 µs (60 Hz) frame budget, an 80×24 viewport backed by a 100,000-row C++ `QAbstractListModel` (Release `-O3`, `pragma ComponentBehavior: Bound`) completes idle layout and paint passes in **< 30 µs** (emitting **0 diff bytes** when unchanged), single-row `dataChanged` updates and repaints in **6.2–22.2 µs**, and 1-row virtualized scroll steps in **~8.8 µs** (`ListView`), **~34.3 µs** (`GridView`), **~20.5–25.7 µs** (`TableView`), and **~8.7–17.9 µs** (`TreeView`). See the [View Benchmark Comparison](docs/contributing/benchmarks/2026-10-27-view-comparison.md), [Model View Performance Profile](docs/contributing/benchmarks/2026-09-11-view-phases.md), and [0.4 Acceptance Benchmarks](docs/contributing/benchmarks/0.4-acceptance/README.md) for methodology and reproduction commands.

## Constraints and Limitations

- **Integer character-cell geometry**: Item coordinates (`x`, `y`, `width`, `height`, margins, padding, spacing) are integer character cells. `Canvas` and `Image` render sub-cell glyphs (2x4 Braille, 1x2 half-blocks) or pixel protocols inside those integer cell bounds.
- **Explicit keyboard quit path**: Terminal applications in raw mode have no window close button, and `TextInput`/`TextArea` use `Ctrl+C` for clipboard copy. Applications must provide a reachable keyboard exit path (such as handling `Q` or `Escape` at the root `Terminal`).
- **Terminal capability variance**: Protocol support depends on the host terminal emulator. `TerminalCapabilities` probes DEC mode 2026 synchronized output, Kitty keyboard protocol, SGR mouse reporting, Kitty/Sixel graphics, and OSC 52 clipboard at startup and falls back when a feature is absent.
- **Flat views vs. hierarchical trees**: `ListView`, `TableView`, `GridView`, and `Repeater` present a single model level (`rootIndex` on `ListView`/`GridView`/`Repeater`, or top-level rows on `TableView`). Hierarchical models require `TreeView`.
- **Optional private Qt headers**: `Behavior` and direct-assignment `PropertyChanges` use `Qt6::QmlPrivate` automatically when available (`QMLTUI_ENABLE_PRIVATE_QT=ON` by default; pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` to opt out), which ties binary builds with private headers to the minor Qt release. `State`, `StateGroup`, `Transition`, map-based `PropertyChanges`, `NumberAnimation`, `ColorAnimation`, and `SequentialAnimation`/`ParallelAnimation` use only public `Qt6::Qml` APIs.
- **POSIX-only `PtyView`**: `QmlTui.PtyView` spawns child processes inside Unix pseudo-terminals (`forkpty`) backed by `libvterm` and is supported only on Linux and macOS (not Windows or WebAssembly).

## Documentation

- **[Getting Started Tutorial](docs/getting-started.md)**: Build a standalone 3-file QmlTui application and expose a C++ `QObject` with `QML_ELEMENT`.
- **[QmlTui for Qt Quick and Widgets Developers](docs/guides/qt-quick-to-qmltui.md)**: Key differences from `QtQuick` and `QtWidgets` (imports, integer cell coordinates, borders, colors, keyboard quit handling, and shared C++ models).
- **[Build Configuration and Installation](docs/build-configuration.md)**: `FetchContent` vs. `find_package(QmlTui)`, optional modules, CMake options, and environment variables.
- **[Qt Creator Setup and Tooling](docs/guides/qt-creator.md)**: Project wizards (`qmltui-qtc`), offline `F1` documentation (`qmltui.qch`), `qmlls`, `qmllint`, and `Runner` actions.
- **[Interactive Playground](docs/playground.md)**: Inspect, edit, and run gallery components in the browser.
- **[Guides](docs/guides/index.md)**: Architecture, layouts, input and focus, models and views, styling, animations and states, inline mode, `PtyView`, custom C++ items, and headless testing with `Runner`.
- **[Examples and Recipes](docs/examples.md)**: Gallery demos and copyable QML/C++ recipes (see [`examples/README.md`](examples/README.md) and standalone applications in [qmltui-dev/qmltui-apps](https://github.com/qmltui-dev/qmltui-apps)).
- **[QML Types Reference](docs/qml-types/index.md)**: API reference for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
- **[C++ Classes Reference](docs/cpp-classes/index.md)**: Public C++ APIs for `Item`, `Terminal`, `TextGridBuffer`, `Cell`, `Color`, `PtyView`, and `Runner`.
- **[Changelog](docs/changes/changelog.md)**: Release history and migration notes.

## License

QmlTui is available under the **LGPLv3**. Examples and the starter template are
**BSD-3-Clause**; documentation is **CC-BY-4.0**. See [LICENSE.md](LICENSE.md).

Separate commercial terms are available upon inquiry for organizations unable
to meet those obligations. Contact through the project repository at
<https://github.com/qmltui-dev/qmltui>.

QmlTui is an independent project and not a product of The Qt Company. Qt and
third-party dependencies remain subject to their own licenses.
