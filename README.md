<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/branding/qmltui_lockup_dark.svg">
    <img alt="QmlTui" src="docs/assets/branding/qmltui_lockup_light.svg" width="360">
  </picture>
</p>

<p align="center">
  <a href="https://github.com/qmltui/qmltui/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/qmltui/qmltui/actions/workflows/ci.yml/badge.svg?branch=main"></a>
  <a href="https://github.com/qmltui/qmltui/blob/badges/summary.md"><img alt="Coverage" src="https://github.com/qmltui/qmltui/raw/badges/coverage.svg"></a>
</p>

<p align="center">
  <a href="https://qmltui.github.io/website/"><strong>Documentation</strong></a> ·
  <a href="https://qmltui.github.io/website/playground.html"><strong>Live Playground (Wasm)</strong></a> ·
  <a href="https://qmltui.github.io/website/playground.html?mode=gallery"><strong>Component Gallery</strong></a> ·
  <a href="https://qmltui.github.io/website/showcase.html"><strong>App Showcase</strong></a>
</p>

**QmlTui** is a retained-mode terminal UI framework for Qt 6.8+ and C++17, targeting Linux, macOS, Windows Terminal, and WebAssembly.

Declare reactive layouts, property bindings, states, and reusable components in QML, backed by C++ models and controllers with `QObject`, `QML_ELEMENT`, and `QAbstractItemModel`.

QmlTui renders the item tree into a double-buffered character-cell grid, diffs consecutive frames, and writes minimal ANSI/SGR updates on `QCoreApplication` without requiring `QtQuick`, `QtGui`, or a display server.

![QmlTui Interactive Component Gallery](docs/assets/casts/gallery-tour.gif)

## Hello, World!

From [`examples/helloworld/`](examples/helloworld/):

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
            event.accepted = true;
            console.log("Goodbye!");
            Qt.quit();
        } else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            event.accepted = true;
            root.count++;
        } else if (event.key === Qt.Key_R) {
            event.accepted = true;
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

![Hello, QmlTui! rendered in the terminal](docs/assets/casts/hello-world.gif)

## Quickstart

Try QmlTui directly in your browser with zero installation in the [Live WebAssembly Playground](https://qmltui.github.io/website/playground.html), browse all built-in controls in the [Component Gallery](https://qmltui.github.io/website/playground.html?mode=gallery), or explore complete applications in the [App Showcase](https://qmltui.github.io/website/showcase.html).

To build locally, install **CMake 3.21+**, a **C++17** compiler, and **Qt 6.8+** (`Core` and `Qml`, tested across Qt 6.8 through 6.12):

```sh
cmake -S . -B build -G Ninja -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

To create a standalone 3-file project (`CMakeLists.txt`, `main.cpp`, `Main.qml`), follow the [Getting Started tutorial](docs/getting-started.md) or copy [`examples/helloworld/`](examples/helloworld/). For browser builds in `xterm.js`, see the [WebAssembly guide](docs/guides/webassembly.md).

## QML Modules and CMake Targets

| QML Import | CMake Target | Dependencies | Summary |
| :--- | :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | `Qt6::Core`, `Qt6::Qml`, `utf8proc` | `Terminal` (`AltScreen`/`Inline`), `Scrollback`, `Item`, `Popup`, `Rectangle`, `Text`, `TextInput`, `TextArea`, layouts, virtualized views, `Canvas`, input, states, animations |
| `import QmlTui.Controls` | `qmltui::controls` | `qmltui::core` | `Button`, `CheckBox`, `RadioButton`, `ComboBox`, `SpinBox`, `Slider`, `ProgressBar`, `BusyIndicator`, `Sparkline`, `TabBar`, `StackView`, `Panel`, `Dialog`, `ToolTip` |
| `import QmlTui.Image` | `qmltui::image` | `qmltui::core`, `Qt6::Gui`, `Qt6::Network` | `Image` and `AnimatedSprite` via Kitty, Sixel, or Unicode half-blocks (`-DQMLTUI_ENABLE_IMAGE=ON`) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | `qmltui::core`, `libvterm` | Embedded Unix pseudo-terminal (`PtyView`) on Linux and macOS (`-DQMLTUI_ENABLE_PTYVIEW=ON`) |

See [Build Configuration and Installation](docs/build-configuration.md) for `FetchContent`, `find_package(QmlTui)`, CMake options, and environment variables.

## Performance and Memory

- **24-byte value-type `Cell`**: [`Cell`](docs/cpp-classes/cell.md) stores UTF-8 grapheme clusters up to 11 bytes inline without per-cell heap allocation. Consecutive `TextGridBuffer` frames are diffed cell by cell and emitted inside DEC mode 2026 synchronized output blocks.
- **Sub-millisecond view updates**: On an 80×24 viewport backed by a 100,000-row C++ `QAbstractListModel` (Release `-O3`, `pragma ComponentBehavior: Bound`), idle layout/paint completes in **< 30 µs** (**0 diff bytes**), single-row `dataChanged` repaints take **6.2–22.2 µs**, and 1-row virtualized scroll steps take **8.7–34.3 µs** across `ListView`, `TreeView`, `TableView`, and `GridView`. See [View Benchmark Comparison](docs/contributing/benchmarks/2026-10-27-view-comparison.md) and [0.4 Acceptance Benchmarks](docs/contributing/benchmarks/0.4-acceptance/README.md).

## Constraints and Limitations

- **Integer character-cell geometry**: Coordinates and dimensions (`x`, `y`, `width`, `height`, margins, padding, spacing) are integer character cells; `Canvas` and `Image` draw sub-cell patterns inside those cell bounds.
- **Explicit keyboard quit path**: Raw-mode terminals have no close button, and `TextInput`/`TextArea` bind `Ctrl+C` to copy. Applications must provide an explicit exit handler (such as `Q` or contextual `Escape` on the root `Terminal`).
- **Terminal capability variance**: `TerminalCapabilities` probes DEC mode 2026, Kitty keyboard, SGR mouse, Kitty/Sixel graphics, and OSC 52 clipboard at startup and degrades cleanly when unsupported.
- **Flat views vs. trees**: `ListView`, `TableView`, `GridView`, and `Repeater` display one model level (`rootIndex` selects a subtree); multi-level trees require `TreeView`.
- **Optional private Qt headers**: `Behavior` and direct-assignment `PropertyChanges` link `Qt6::QmlPrivate` when available (`QMLTUI_ENABLE_PRIVATE_QT=ON` by default, coupling shared binaries to that `Qt 6.x` minor release; pass `OFF` for public-Qt-only builds).
- **POSIX-only `PtyView`**: `QmlTui.PtyView` uses `forkpty` and `libvterm` and is supported on Linux and macOS only.

## Documentation

- **[Documentation Portal](https://qmltui.github.io/website/)** · **[Live Playground (Wasm)](https://qmltui.github.io/website/playground.html)** · **[Component Gallery](https://qmltui.github.io/website/playground.html?mode=gallery)** · **[App Showcase](https://qmltui.github.io/website/showcase.html)**
- **[Getting Started](docs/getting-started.md)** · **[Qt Quick & Widgets Guide](docs/guides/qt-quick-to-qmltui.md)** · **[Build & CMake](docs/build-configuration.md)** · **[Editor Setup](docs/guides/editor-setup.md)**
- **[Guides](docs/guides/index.md)** · **[Testing with Runner](docs/guides/testing.md)** · **[Agentic Loop](docs/guides/agentic-development.md)** · **[Examples & Recipes](docs/examples.md)** ([qmltui/apps](https://github.com/qmltui/apps))
- **[QML Types Reference](docs/qml-types/index.md)** · **[C++ Classes Reference](docs/cpp-classes/index.md)** · **[Changelog](docs/changes/changelog.md)** · **[Contributing](CONTRIBUTING.md)**

## License

QmlTui is available under the **GNU Lesser General Public License v3.0 (`LGPL-3.0-only`)**. Examples and the starter template are **BSD-3-Clause**; documentation is **CC-BY-4.0**. See [LICENSE.md](LICENSE.md).

QmlTui is an independent open-source project and not a product of The Qt Company.
