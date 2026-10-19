---
layout: home
title: QmlTui
titleTemplate: Retained Declarative QML Terminal UIs + High-Performance C++20 / Qt 6 Backend

hero:
  name: QmlTui
  text: Declarative QML Terminal UIs on a High-Performance C++20 & Qt 6 Engine
  tagline: Combine the ergonomics of a retained-mode declarative QML scene graph and reactive property bindings with native C++20 backends, 1M-row QAbstractItemModel virtualization, and O(viewport) double-buffered cell diffing.
  image:
    light: /assets/branding/qamui_mark_light.svg
    dark: /assets/branding/qamui_mark.svg
    alt: QmlTui
  actions:
    - theme: brand
      text: Get Started →
      link: ./getting-started
    - theme: alt
      text: ▶ Try Live Playground
      link: ./playground
    - theme: alt
      text: Guides
      link: ./guides/
    - theme: alt
      text: 39 Components
      link: ./examples

features:
  - icon: ⚡
    title: Retained Declarative QML
    details: Declare layouts, borders, focus chains, and states once. Fine-grained Qt property bindings update only the affected cells—no immediate-mode full-tree rebuilds on every tick.
    link: ./guides/architecture
    linkText: Retained architecture
  - icon: 🚀
    title: High-Performance C++20 & Qt 6 Backend
    details: Drive UIs directly from compiled C++20 QObjects, worker threads, and 1M-row QAbstractItemModels with O(viewport) virtualized views and double-buffered cell diffing.
    link: ./guides/models
    linkText: Models & views
  - icon: 🧱
    title: Complete Core in a Single Import
    details: "import QmlTui provides Terminal, Rectangle, layouts, Text, TextInput, virtualized ListView/TableView/TreeView, Canvas, and animations standalone—with zero QtQuick dependency."
    link: ./guides/architecture#module-boundaries-what-qmltui-core-provides-alone
    linkText: What QmlTui.Core provides
  - icon: 🖥️
    title: AltScreen & Inline Modes
    details: Run full-screen alternate-screen dashboards (Terminal.AltScreen) or streaming CLI workflows (Terminal.Inline) with native Scrollback history.
    link: ./guides/inline-mode
    linkText: Explore inline mode
  - icon: 🎨
    title: Modern Terminal Protocols
    details: Automatic capability detection for DEC ?2026 sync output, Kitty keyboard, Kitty & Sixel graphics, Canvas Braille/half-block drawing, and SGR mouse.
    link: ./guides/styling-and-colors
    linkText: Styling & protocols
  - icon: 🧪
    title: Deterministic Headless Testing
    details: Drive applications headlessly with a virtual clock, assert committed cell and style frames in CTest, or record Asciinema .cast demos.
    link: ./guides/testing
    linkText: Testing guide
---

![QmlTui Interactive Gallery](assets/casts/gallery-tour.gif)

## Why Retained Declarative QML + C++20 / Qt 6?

Most terminal UI toolkits force a trade-off: write verbose immediate-mode layout loops that rebuild the entire widget tree on every keystroke, or use an interpreted runtime that bottlenecks on high-frequency telemetry and large datasets. **QmlTui pairs a declarative QML frontend with a native C++20/Qt 6 engine:**

1. **Retained Scene Graph & Surgical Bindings**: Items (`Rectangle`, `RowLayout`, `Text`, `ListView`) live in a persistent C++ cell tree. When a property changes, Qt's binding engine re-evaluates only the dependent expressions and marks only those subtrees dirty.
2. **Native C++20 & `QAbstractItemModel` Throughput**: Expose C++20 services (`QML_ELEMENT`, `Q_PROPERTY`) and streaming models directly to QML with zero serialization glue. Virtualized views (`TableView`, `TreeView`, `ListView`, `GridView`) instantiate delegates only for the visible terminal rows (`O(viewport)`), keeping frame times sub-millisecond even over 1,000,000 rows.
3. **Minimal ANSI Diffs (`TextGridBuffer`)**: On each dirty tick, QmlTui composites the retained tree into a contiguous double-buffered cell grid, diffs cell-by-cell against the previous committed frame, and emits only the changed glyphs and SGR sequences inside a single DEC `?2026` synchronized output frame.

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

![Hello, QmlTui! rendered in the terminal](assets/casts/hello-world.gif)

Try QmlTui locally in 3 commands or launch the **[Interactive Browser Playground](playground.md)**:

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

---

## Start Here

New to QmlTui? Follow these guides to set up your environment, run the interactive gallery, and build your first application:

- **[Getting Started](getting-started.md)**: System requirements, running `qmltui_gallery`, creating a standalone 3-file application (`FetchContent` or `find_package`), CMake options, and environment variables.
- **[Interactive Playground](playground.md)**: Inspect, edit, and run all 39 gallery components directly in your browser with QmlTui autocomplete.
- **[Examples & Component Cookbook](examples.md)**: Understand the four-tier `examples/` organization (`QmlTui.Controls` vs. copyable QML recipes) and browse every recorded component and guide demo.
- **[IDE Setup](guides/ide-setup.md)**: Configure `qmlls`, `qmllint`, and `qmltui-runner.d.ts` in Qt Creator, VS Code, Neovim, and Helix for full autocomplete and type checking.
- **[QmlTui for Qt Quick Developers](guides/qt-quick-to-qmltui.md)**: Map your `QtQuick` mental model to integer cell coordinates, terminal borders, `AltScreen` vs. `Inline` mode, and zero-`QtQuick` imports.

## Guides

Deep-dive into QmlTui's subsystems and architectural patterns (or view the [Guides index](guides/index.md)):

- **[Architecture](guides/architecture.md)**: Retained item tree, double-buffered `TextGridBuffer` diffing, platform backends (`PosixTerminalBackend`, `Win32ConsoleBackend`), and what `QmlTui.Core` provides standalone vs. companion modules.
- **[Layouts & Cell Geometry](guides/layouts.md)**: Integer cell coordinates, `anchors`, `Row`/`Column` positioners, `RowLayout`/`ColumnLayout`/`GridLayout`/`StackLayout` engines.
- **[Input & Focus](guides/input-and-focus.md)**: Active focus chains, `Keys.onPressed` bubbling, `Shortcut`, SGR mouse events, and bracketed paste.
- **[Styling & Colors](guides/styling-and-colors.md)**: Terminal-default colors, ANSI 16/256/TrueColor quantization, `Gradient`, `Border` styles, and application-owned QML component styling.
- **[Models in QmlTui Views](guides/models.md)**: Connecting `ListModel` and C++ `QAbstractItemModel` subclasses to virtualized `ListView`, `TableView`, `TreeView`, and `GridView`.
- **[Animation & States](guides/animation-and-states.md)**: `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, `State`, `Transition`, and Private Qt `Behavior` / `StateGroup`.
- **[Inline Mode & Scrollback](guides/inline-mode.md)**: Building streaming CLIs and coding agents with `Terminal.Inline` and `Scrollback` alongside a live bottom viewport.
- **[Embedded PTY Shells (`QmlTui.TerminalHost`)](guides/terminal-host.md)**: Hosting interactive login shells and CLI processes inside `TerminalView` on Linux and macOS.
- **[Custom C++ Items](guides/custom-items.md)**: Subclassing `qmltui::Item` and painting into `TextGridBuffer` in C++, exposing QML types with `QML_ELEMENT`, or building C++-only scenes.
- **[Testing & Recording with Runner](guides/testing.md)**: Deterministic headless testing with a virtual clock, golden frame/style snapshots, Asciinema `.cast` recording, and `qmltui-runner`.
- **[WebAssembly Playground](guides/webassembly.md)**: Compiling and embedding QmlTui applications in the browser.

## API Reference

- **[QML Types Reference](qml-types/index.md)**: Complete property, method, and signal documentation for `QmlTui`, `QmlTui.Controls`, `QmlTui.Image`, and `QmlTui.TerminalHost`.
- **[C++ Classes Reference](cpp-classes/index.md)**: Public C++ headers for `Item`, `Terminal`, `TerminalEngine`, `TextGridBuffer`, `Cell`, `Color`, `TerminalCapabilities`, `TerminalView`, and `Runner`.
- **[Release Notes & Changelog](changes/changelog.md)**: Version history, new features, and migration guides.

## Contributing & Internals

- **[CI Runners](contributing/ci-runners.md)**: Continuous integration matrix and headless test execution.
- **[Recording Demos](contributing/recording.md)**: Capturing deterministic `.cast` recordings and GIF assets with `Runner`.
- **[Development Plans](contributing/plans/README.md)**: Design documents and milestone architecture records.
