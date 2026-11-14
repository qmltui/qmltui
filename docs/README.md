# QmlTui Documentation

**QmlTui** is a retained-mode terminal UI framework for Qt 6.8+ and C++17. Declare layouts, box-drawing borders, focus chains, states, and property bindings in QML while keeping models and business logic in C++ (`QCoreApplication`, `QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`), without requiring `QtQuick`, `QtGui`, or a display server.

## Hello, World!

With `import QmlTui`, declare a bordered card, reactive property bindings, and keyboard handlers:

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

Rendered in the terminal:

![Hello, QmlTui! rendered in the terminal](assets/casts/hello-world.gif)

## Interactive Component Gallery

Browse 45 interactive controls, views, layouts, animations, and terminal capabilities in `qmltui_gallery`:

![QmlTui Interactive Component Gallery](assets/casts/gallery-tour.gif)

To build and run the component gallery locally:

```sh
cmake -S . -B build -G Ninja -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

## Start Here

- **Build your first terminal app**: Follow the [Getting Started Tutorial](getting-started.md) to create a standalone CMake project, bind a C++ `QObject` with `QML_ELEMENT`, and run a headless `Runner` test in CTest (or copy [`examples/helloworld/`](../examples/helloworld/)).
- **Coming from Qt Quick or Qt Widgets**: Read [QmlTui for Qt Quick and Widgets Developers](guides/qt-quick-to-qmltui.md) and [Models in QmlTui Views](guides/models.md) for imports, integer cell coordinates, borders, colors, keyboard quit paths, and shared C++ models.
- **Configure CMake, editors, and CI tests**: Set up `FetchContent` or `find_package(QmlTui)` in [Build Configuration and Installation](build-configuration.md), enable `qmlls` and `Runner` autocomplete in [Editor and IDE Setup](guides/editor-setup.md), and write snapshot tests in [Testing and Recording with Runner](guides/testing.md).
- **Explore examples and full-scale applications**: Browse focused recipes in [Examples and Recipes](examples.md) and 12 complete showcase applications (`btop`, `agent_stream`, `file_manager`, `package_manager`, `audio_player`, and more) in [qmltui/apps](https://github.com/qmltui/apps).

## Guides and API Reference

- **[Guides Index](guides/index.md)**: Recommended reading order across architecture, layouts, input and focus, styling, models, animation and states, inline mode, `PtyView`, custom C++ items, `Runner` testing, editor setup, and WebAssembly.
- **[QML Types Reference](qml-types/index.md)**: Properties, methods, and signals for all 75 types across `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Terminal`, `Item`, `TextGridBuffer`, `Cell`, `Color`, `TerminalCapabilities`, `PtyView`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history and migration notes.

## Constraints and Limitations

- **Integer character-cell geometry**: Item coordinates (`x`, `y`, `width`, `height`, margins, padding, spacing) are integer character cells. `Canvas` and `Image` render sub-cell glyphs (2x4 Braille, 1x2 half-blocks) or pixel protocols inside those integer cell bounds.
- **Explicit keyboard quit path**: Terminal applications in raw mode have no window close button, and `TextInput`/`TextArea` use `Ctrl+C` for clipboard copy. Applications must provide a reachable keyboard exit path (such as handling `Q` or `Escape` at the root `Terminal`).
- **Terminal capability variance**: Protocol support depends on the host terminal emulator. `TerminalCapabilities` probes DEC mode 2026 synchronized output, Kitty keyboard protocol, SGR mouse reporting, Kitty/Sixel graphics, and OSC 52 clipboard at startup and degrades cleanly when a feature is absent (for example, `Image.Auto` falls back from Kitty to Sixel to Unicode half-blocks).
- **Platform targets**: Runtime targets are Linux, macOS, and Windows Terminal (`Win32ConsoleBackend` over Virtual Terminal sequences), plus browsers via Qt for WebAssembly (`WasmTerminalBackend`). Legacy `conhost.exe` receives whatever its VT layer supports and is not a target.
- **Flat views vs. hierarchical trees**: `ListView`, `TableView`, `GridView`, and `Repeater` present a single model level (`rootIndex` on `ListView`/`GridView`/`Repeater`, or top-level rows on `TableView`). Hierarchical models require `TreeView`.
- **Optional private Qt headers**: `Behavior` and direct-assignment `PropertyChanges` (plus automatic binding restoration on state exit) use `Qt6::QmlPrivate` when available (`QMLTUI_ENABLE_PRIVATE_QT=ON` by default; pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` to opt out), which ties binary builds with private headers to the minor Qt release. `State`, `StateGroup`, `Transition`, map-based `PropertyChanges`, `NumberAnimation`, and `ColorAnimation` use only public `Qt6::Qml` APIs.
- **POSIX-only `PtyView`**: `QmlTui.PtyView` spawns child processes inside Unix pseudo-terminals (`forkpty`) backed by `libvterm` and is supported only on Linux and macOS (not Windows or WebAssembly).

## Contributing

- **[CI Runners](contributing/ci-runners.md)**: Continuous integration setup and headless test execution.
- **[Recording Demos](contributing/recording.md)**: Capturing `.cast` recordings and GIF assets with `Runner`.
- **[Development Plans](contributing/plans/README.md)**: Design notes and milestone records.
