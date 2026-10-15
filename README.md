<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/branding/qmltui_lockup_dark.svg">
    <img alt="QmlTui" src="docs/assets/branding/qmltui_lockup_light.svg" width="360">
  </picture>
</p>

**QmlTui** is a C++20 and Qt 6.8+ retained-mode terminal UI framework that brings declarative QML, property bindings, and Qt model/view architecture to modern terminal emulators. Build responsive full-screen dashboards or inline streaming CLIs across Linux, macOS, and Windows Terminal with zero `QtQuick` or display-server dependencies.

### Key Highlights

- **Declarative QML & Reactive Bindings**: Compose terminal interfaces with `QtQml` and `QmlTui` modules, integer cell geometry, states, animations, and type-safe property bindings—without linking `QtQuick` or a GUI window system.
- **Dual Rendering Modes (`Frame` & `Stream`)**: Run full-screen alternate-screen applications (`Terminal.Frame`) or inline CLI workflows (`Terminal.Stream`) that push completed prompts and cards into native terminal history via `Scrollback`.
- **1M-Row Virtualized Model/View**: Bind C++ `QAbstractItemModel` datasets directly to virtualized `TableView`, `TreeView`, `ListView`, `GridView`, and `LogView` controls with O(viewport) per-frame cell diffing.
- **Modern Terminal Protocols**: Automatic capability detection for DEC `?2026` synchronized output, Kitty keyboard disambiguation, Kitty and Sixel graphics (`Image`, `AnimatedSprite`, `Canvas`), SGR mouse tracking, OSC 8 clickable links, OSC 9;4 progress reporting, and an optional Unix PTY `TerminalView` host.
- **Deterministic Headless Testing (`Runner`)**: Drive applications headlessly with a virtual clock, assert committed cell and style snapshots in CTest, or record deterministic Asciinema v2 `.cast` demos without a live TTY.

![Gallery tour](docs/assets/casts/gallery-tour.gif)

## What It Looks Like

Declare your state, layout, and controls in QML; QmlTui handles focus navigation, Unicode box drawing, and minimal ANSI cell diffs automatically:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    property int usedMiB: 512

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Panel {
        anchors.centerIn: parent
        width: 44
        height: 9
        title: "Pod Metrics"
        titleBadge: "[RUNNING]"
        titleBadgeColor: "green"
        titleTrailing: "v0.5.0"
        footer: "[q/Esc] Quit"
        footerTrailing: "CPU: 42%"
        border.style: Border.Rounded

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Text { text: "Memory: " + root.usedMiB + " / 2048 MiB" }
            ProgressBar {
                width: 42
                to: 2048
                value: root.usedMiB
                showBrackets: true
                showPercentage: true
                color: "cyan"
            }
            Button {
                text: "Scale +256 MiB"
                focus: true
                onClicked: root.usedMiB = Math.min(2048, root.usedMiB + 256)
            }
        }
    }
}
```

Rendered in the terminal:

```text
╭─ Pod Metrics [RUNNING] ───────── v0.5.0 ─╮
│Memory: 512 / 2048 MiB                    │
│                                          │
│[█████████░░░░░░░░░░░░░░░░░░░░░░░░░░]  25%│
│                                          │
│╔════════════════╗                        │
│║ Scale +256 MiB ║                        │
│╚════════════════╝                        │
╰─ [q/Esc] Quit ──────────────── CPU: 42% ─╯
```

## Quickstart

### 1. Run the Interactive Component Gallery

Requires CMake 3.21+, a C++20 compiler, and Qt 6.8+ (`Core` and `Qml`; plus `Gui` and `Network` for the default `QmlTui.Image` module):

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

Or try the [WebAssembly playground](docs/topics/webassembly.md) in your browser.

### 2. Build a Standalone 3-File App

Follow [Getting started](docs/getting-started.md) (or copy [`examples/quickstart/`](examples/quickstart/)) to bootstrap a standalone `CMakeLists.txt` + `main.cpp` + `Main.qml` project using CMake `FetchContent` or `find_package(QmlTui)`.

## Documentation

- **[Getting started](docs/getting-started.md)**: Requirements, 3-file standalone quickstart, `find_package(QmlTui)`, CMake options, and runtime environment variables.
- **[Six-part tutorial](docs/tutorials/index.md)**: Step-by-step guide from your first `Terminal` to layouts, input, models, controls, and headless `Runner` testing.
- **[Topics & guides](docs/topics/index.md)**: Architecture, layouts, input/focus, virtualized models, styling, inline/stream mode, custom C++ items, and comparisons for Qt Quick and TUI developers.
- **[Component & recipe catalog](docs/examples.md)**: Controls vs. copyable QML recipes, runnable component demos, and topic examples (see also [`examples/README.md`](examples/README.md) and full showcases in [qmltui-apps](https://github.com/qmltui-dev/qmltui-apps)).
- **[QML types reference](docs/qml-types/index.md)**: Complete API reference for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.TerminalHost`.
- **[C++ classes reference](docs/cpp-classes/index.md)**: Engine, custom `Item` subclassing, rendering primitives, and `Runner` APIs.
- **[Changelog & release notes](docs/changes/changelog.md)**: Version history and migration notes.

See the [documentation home page](docs/README.md) for the full reading order by background.

## License

QmlTui is available under **LGPL-3.0-only OR GPL-3.0-only**. Examples and the
starter template are **BSD-3-Clause**; documentation is **CC-BY-4.0**. See
[LICENSE.md](LICENSE.md).

Separate commercial terms are available upon inquiry for organizations unable
to meet those obligations. Contact through the project repository at
<https://github.com/qmltui-dev/qmltui>.

QmlTui is an independent project and not a product of The Qt Company. Qt and
third-party dependencies remain subject to their own licenses.
