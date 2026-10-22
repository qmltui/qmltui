---
layout: home
title: QmlTui
titleTemplate: Retained-Mode Terminal UI Framework for Qt 6

hero:
  name: QmlTui
  text: Retained-Mode Terminal UIs with QML and Qt 6
  tagline: Build terminal applications with QML layouts, property bindings, and C++ QAbstractItemModels on a double-buffered character-cell renderer.
  image:
    light: /assets/branding/qmltui_mark_light.svg
    dark: /assets/branding/qmltui_mark.svg
    alt: QmlTui
  actions:
    - theme: brand
      text: Get Started
      link: ./getting-started
    - theme: alt
      text: Playground
      link: ./playground
    - theme: alt
      text: Guides
      link: ./guides/
    - theme: alt
      text: Examples
      link: ./examples

features:
  - title: Retained QML Scene Graph
    details: Declare layouts, borders, focus chains, and states in QML. Property bindings mark only the affected subtrees dirty and diff the resulting cell grid against the previous frame.
    link: ./guides/architecture
    linkText: Architecture
  - title: C++ Models and Views
    details: Drive virtualized ListView, TableView, TreeView, and GridView components from C++ QAbstractItemModel and QSortFilterProxyModel instances without QtQuick.
    link: ./guides/models
    linkText: Models & views
  - title: AltScreen and Inline Modes
    details: Run full-screen terminal interfaces (Terminal.AltScreen) or inline CLI sessions (Terminal.Inline) that commit history to native terminal scrollback.
    link: ./guides/inline-mode
    linkText: Inline mode
  - title: Headless Testing with Runner
    details: Drive keyboard and mouse input deterministically, step a virtual clock, assert committed text and style snapshots in CTest, and record Asciinema .cast files.
    link: ./guides/testing
    linkText: Testing guide
---

![QmlTui Interactive Gallery](assets/casts/gallery-tour.gif)

## Hello, World!

With `import QtQml` and `import QmlTui`, declare a bordered card, reactive state bindings, and keyboard handlers:

```qml
import QtQml
import QmlTui

Terminal {
    id: root
    property int count: 0

    title: count === 0 ? qsTr("Hello QmlTui!") : qsTr("Interactions: %1").arg(root.count)
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
        color: "#161b22"
        border {
            style: Border.Single
            color: "#2cde85"
        }

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                text: qsTr("Hello, QmlTui!")
                color: "#00ff66"
                bold: true
            }

            Text {
                text: qsTr("QML Declarative terminal UIs")
                color: "#2cde85"
                dim: true
            }

            Text {
                text: qsTr("Interactions: %1").arg(root.count)
                color: "#c9d1d9"
            }

            Text {
                text: qsTr("[Enter] Count  •  [R] Reset  •  [Q] Quit")
                color: "#8b949e"
                dim: true
            }
        }
    }
}
```

Rendered in the terminal:

![Hello, QmlTui! rendered in the terminal](assets/casts/hello-world.gif)

Build and run the component gallery locally, or open the [WebAssembly Playground](playground.md):

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

## Start Here

- **[Getting Started](getting-started.md)**: System requirements, running `qmltui_gallery`, creating a standalone 3-file application (`FetchContent` or `find_package(QmlTui)`), CMake options, and environment variables.
- **[Interactive Playground](playground.md)**: Edit and run gallery components in the browser with QmlTui autocomplete.
- **[Examples and Recipes](examples.md)**: Browse the built-in component gallery and standalone QML/C++ recipes.
- **[Qt Creator Setup](guides/qt-creator.md)**: Configure project wizards (`qmltui-qtc`), offline `F1` help (`qmltui.qch`), `qmlls`, `qmllint`, and `Runner` build actions.
- **[QmlTui for Qt Quick Developers](guides/qt-quick-to-qmltui.md)**: Differences between `QtQuick` and QmlTui's character-cell coordinate system, borders, terminal modes, and imports.

## Guides

See the [Guides index](guides/index.md) for the full reading order:

- **[Architecture](guides/architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` diffing, platform backends, and QML module boundaries.
- **[Layouts & Cell Geometry](guides/layouts.md)**: Integer cell coordinates, `anchors`, `Row`/`Column` positioners, and `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout`.
- **[Input & Focus](guides/input-and-focus.md)**: Focus chains, `FocusScope`, `Keys.onPressed` propagation, `Shortcut`, SGR mouse events, and bracketed paste.
- **[Styling & Colors](guides/styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient`, `Border` styles, and reusable QML component styling.
- **[Models in QmlTui Views](guides/models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to `ListView`, `TableView`, `TreeView`, and `GridView`.
- **[Animation & States](guides/animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `Transition`, and optional `Behavior` / `StateGroup`.
- **[Inline Mode & Scrollback](guides/inline-mode.md)**: Building streaming CLIs and REPLs with `Terminal.Inline` and `Scrollback`.
- **[Embedded PTY Shells (`QmlTui.PtyView`)](guides/pty-view.md)**: Hosting interactive Unix child processes inside `PtyView` on Linux and macOS.
- **[Custom C++ Items](guides/custom-items.md)**: Subclassing `qmltui::Item`, painting into `TextGridBuffer`, registering QML types with `QML_ELEMENT`, and C++-only scenes.
- **[Testing & Recording with Runner](guides/testing.md)**: Headless testing with a virtual clock, text and style snapshots, and Asciinema `.cast` recording.
- **[WebAssembly Playground](guides/webassembly.md)**: Building and hosting QmlTui applications in the browser with Qt for WebAssembly.

## API Reference

- **[QML Types Reference](qml-types/index.md)**: Properties, methods, and signals for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.PtyView`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Item`, `Terminal`, `TerminalEngine`, `TextGridBuffer`, `Cell`, `Color`, `TerminalCapabilities`, `PtyView`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history and migration notes.

## Constraints & Limitations

- **Integer character-cell geometry**: Item coordinates (`x`, `y`, `width`, `height`, margins, padding, spacing) are integer character cells. `Canvas` and `Image` render sub-cell glyphs (2x4 Braille, 1x2 half-blocks) or pixel protocols inside those integer cell bounds.
- **Terminal capability variance**: Protocol support depends on the host terminal emulator. `TerminalCapabilities` probes DEC mode 2026 synchronized output, Kitty keyboard protocol, SGR mouse reporting, Kitty/Sixel graphics, and OSC 52 clipboard at startup and degrades when a feature is absent (for example, `Image.Auto` falls back from Kitty to Sixel to Unicode half-blocks).
- **Platform targets**: Runtime targets are Linux, macOS, and Windows Terminal (`Win32ConsoleBackend` over Virtual Terminal sequences). Legacy `conhost.exe` receives whatever its VT layer supports and is not a target.
- **Flat views vs. hierarchical trees**: `ListView`, `TableView`, `GridView`, and `Repeater` present a single model level (`rootIndex` on `ListView`/`GridView`/`Repeater`, or top-level rows on `TableView`). Hierarchical models require `TreeView`.
- **Optional private Qt headers**: `Behavior` and `StateGroup` require `QMLTUI_ENABLE_PRIVATE_QT=ON` (`Qt6::QmlPrivate`), which ties binary builds of that option to the minor Qt release. `State`, `Transition`, `PropertyChanges`, `NumberAnimation`, `ColorAnimation`, and `SequentialAnimation`/`ParallelAnimation` use only public `Qt6::Qml` APIs.
- **POSIX-only `PtyView`**: `QmlTui.PtyView` spawns child processes inside Unix pseudo-terminals (`forkpty`) backed by `libvterm` and is supported only on Linux and macOS (not Windows or WebAssembly).

## Contributing

- **[CI Runners](contributing/ci-runners.md)**: Continuous integration setup and headless test execution.
- **[Recording Demos](contributing/recording.md)**: Capturing `.cast` recordings and GIF assets with `Runner`.
- **[Development Plans](contributing/plans/README.md)**: Design notes and milestone records.
