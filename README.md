<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/branding/qamui_lockup_dark.svg">
    <img alt="QmlTui" src="docs/assets/branding/qamui_lockup_light.svg" width="360">
  </picture>
</p>

**QmlTui** pairs a **retained-mode declarative QML frontend** with a **high-performance C++20 and Qt 6.8+ backend** for modern terminal emulators. Declare your cell layout, borders, focus chains, and states once in QML while driving high-frequency telemetry, worker threads, and 1M-row `QAbstractItemModel` datasets from native C++20—with O(viewport) virtualization, double-buffered cell diffing, and zero `QtQuick` or display-server dependencies.

### Why Retained Declarative QML + C++20 / Qt 6?

- **Retained Scene Graph & Surgical Bindings**: Unlike immediate-mode TUI libraries that rebuild and re-layout the entire widget tree on every keystroke or tick, QmlTui keeps a persistent C++ cell tree in memory. Fine-grained Qt property bindings re-evaluate only the dependent expressions and mark only affected subtrees dirty.
- **High-Performance C++20 & `QAbstractItemModel` Backend**: Expose compiled C++20 `QObject` services (`QML_ELEMENT`, `Q_PROPERTY`) and streaming `QAbstractItemModel`s directly to QML with zero serialization glue. Virtualized `TableView`, `TreeView`, `ListView`, and `GridView` items instantiate delegates only for visible terminal rows (`O(viewport)`), keeping frame times sub-millisecond over 1,000,000 rows.
- **Complete Core Engine in a Single Import**: `import QmlTui` (`QmlTui.Core`) provides `Terminal`, `Rectangle`, layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`), `Text`, `TextInput`, virtualized views, `Canvas`, and animations standalone; add `QmlTui.Controls`, `QmlTui.Image`, or `QmlTui.PtyView` only when needed.
- **Dual Rendering Modes (`AltScreen` & `Inline`)**: Run full-screen alternate-screen applications (`Terminal.AltScreen`) or inline CLI workflows (`Terminal.Inline`) that push completed prompts and cards into native terminal history via `Scrollback`.
- **Double-Buffered Cell Diffing & Modern Terminal Protocols**: QmlTui composites dirty subtrees into a contiguous `TextGridBuffer`, diffs cell-by-cell against the previous frame, and emits minimal ANSI/SGR sequences inside a DEC `?2026` synchronized frame—with automatic support for Kitty keyboard, Kitty & Sixel graphics, SGR mouse, and OSC 8/9;4.
- **Deterministic Headless Testing**: Drive applications headlessly with a virtual clock, assert committed cell and style snapshots in CTest, or record deterministic Asciinema v2 `.cast` demos without a live TTY (see [Runner](docs/cpp-classes/runner.md)).

![Gallery tour](docs/assets/casts/gallery-tour.gif)

## Hello, World!

With just `import QtQml` and `import QmlTui`, declare a centered bordered card and keyboard quit handler—QmlTui handles cell layout, Unicode box drawing, and minimal ANSI diffs automatically:

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 30
        height: 4
        border {
            style: Border.Rounded
            color: "cyan"
        }

        Column {
            anchors.centerIn: parent

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Hello, QmlTui!"
                color: "cyan"
                bold: true
            }
            Text {
                text: "Press [q] or [Esc] to quit"
                dim: true
            }
        }
    }
}
```

Rendered in the terminal:

![Hello, QmlTui! rendered in the terminal](docs/assets/casts/hello-world.gif)

## Quickstart

### 1. Run the Interactive Component Gallery

Requires CMake 3.21+, a C++20 compiler, and Qt 6.8+ (`Core` and `Qml`; plus `Gui` and `Network` for the default `QmlTui.Image` module):

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

Or launch the [Interactive Playground](docs/playground.md) in your browser (see also the [WebAssembly build guide](docs/guides/webassembly.md)).

### 2. Build a Standalone 3-File App

Follow [Getting started](docs/getting-started.md) (or copy [`examples/quickstart/`](examples/quickstart/)) to bootstrap a standalone `CMakeLists.txt` + `main.cpp` + `Main.qml` project using CMake `FetchContent` or `find_package(QmlTui)`.

## Documentation

- **[Getting started](docs/getting-started.md)**: Requirements, 3-file standalone quickstart, `find_package(QmlTui)`, CMake options, and runtime environment variables.
- **[Interactive playground](docs/playground.md)**: Inspect, edit, and run all 39 gallery components directly in your browser with QmlTui autocomplete.
- **[Guides](docs/guides/index.md)**: Architecture, layouts, input/focus, virtualized models, styling, animations/states, inline/scrollback mode, embedded PTY (`PtyView`), custom C++ items, testing with `Runner`, and Qt Quick migration.
- **[Component & recipe catalog](docs/examples.md)**: Controls vs. copyable QML recipes, runnable component demos, and guide examples (see also [`examples/README.md`](examples/README.md) and full showcases in [qml-tui/apps](https://github.com/qml-tui/apps)).
- **[QML types reference](docs/qml-types/index.md)**: Complete API reference for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
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
