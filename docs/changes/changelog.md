# Changes in 0.6.0

QmlTui 0.6.0 is the first public release of QmlTui for Qt 6.8+ and C++17 across
Linux, macOS, Windows Terminal, and WebAssembly (`xterm.js`).

## Engine, Controls, and Platform Support

- **Core spine encapsulation and partial-damage rendering**: `Item`, `Terminal`, `Popup`, and `FocusScope` now encapsulate internal state behind `d_ptr` (`ItemPrivate`, etc.), and `TerminalEngine` / `AltScreenRenderer` track localized damage rectangles with offscreen subtree culling to reduce per-frame repaint overhead.
- **Normalized QML APIs and `Bound` controls**: Built-in `QmlTui.Controls` components use `pragma ComponentBehavior: Bound` with consistent explicit color override behavior across focus and disabled states, hardened `State` / `PropertyChanges` lifetimes, and bounded glyph interning.
- **`Text`, `Span`, and `HighlightRange` refinements**: Removed ad-hoc inline Markdown (`**...**`, `` `...` ``, `[label](url)`) from `Text.AutoText`/`Text.RichText` so literal globs and logs are not mangled (full CommonMark/GFM rendering now lives in the [`QmlTui.Markdown`](https://github.com/qmltui/markdown) addon), and added `strikethrough` (SGR 9/29, `<s>`/`<del>`) plus optional OSC 8 `link` support to `Span` and `HighlightRange`.
- **`Terminal.Inline` dynamic viewport resizing**: `InlineRenderer` and `Runner::resize(columns, rows)` now reflow `Terminal.Inline` sessions cleanly, keeping frame height content-sized up to `rows - 1`.
- **WebAssembly shell (`qmltui_wasm_app()`)**: Generates a standalone `xterm.js` terminal host page (`<target>.html`) with `@xterm/addon-unicode-graphemes` (`15-graphemes`), OSC 4/10/11 color query replies, and OSC 52 clipboard bridging.
- **`QmlTui.PtyView` extracted into an official add-on**: Moved the Unix-only `QmlTui.PtyView` module and `libvterm` integration out of the core repository into the standalone BSD-3-Clause [`qmltui/ptyview`](https://github.com/qmltui/ptyview) repository alongside `qmltui/tvision`, `qmltui/markdown`, and `qmltui/charts`.
- **Python (`PySide6`) integration**: Added [`examples/recipes/pyside6/`](../../examples/recipes/pyside6/) and the [Python (PySide6) Guide](../guides/python-pyside6.md) demonstrating hosting QmlTui QML modules from Python `QAbstractTableModel` and `QSortFilterProxyModel` backends.

## Migrating from 0.5 to 0.6

| 0.5 API / Target | 0.6 Replacement |
| :--- | :--- |
| `qmltui::core`, `qmltui::controls`, `qmltui::image`, `qmltui::runner` | `QmlTui::Core`, `QmlTui::Controls`, `QmlTui::Image`, `QmlTui::Runner` (plus `QmlTui::qmltui-runner`) |
| `qmltui::ptyview` (`QMLTUI_ENABLE_PTYVIEW=ON`) | Consume `QmlTui::PtyView` from the official [`qmltui/ptyview`](https://github.com/qmltui/ptyview) add-on via `FetchContent` |
| `qmltui::controls::ToolTip` (C++ namespace) | `qmltui::ToolTip` |
| Direct member layout on `Item`, `Terminal`, `Popup`, `FocusScope` | Encapsulated behind `d_ptr` (`ItemPrivate`, etc.) — recompile all C++ subclasses against 0.6 headers |
| Versioned QML imports (`import QmlTui 1.0`) | Unversioned QML imports (`import QmlTui`, `import QmlTui.Controls`, `import QmlTui.Image`) |
| `find_package(QmlTui 0.5 REQUIRED)` | `find_package(QmlTui 0.6 REQUIRED)` (per-module `COMPONENTS` export sets isolate optional `Image` dependencies) |

## Headless Testing (`Runner`) and Agentic Tooling

- **Multi-viewport probes and frame inspection in `qmltui-runner`**: Added `--sweep` (`60x18,80x24,120x36`), `--sizes <WxH,...>`, `--idle-ms <ms>` idle-repaint budget verification, `--ruler` 0-based `(x, y)` coordinate rulers, `--print`, `--styled`, `--scrollback`, and `--clean-artifacts`. See the [Runner API](../cpp-classes/runner.md) and [Agentic Development Guide](../guides/agentic-development.md).
- **Bundled portable coding-agent skills**: Added [`qmltui-qml`](../../.agents/skills/qmltui-qml/SKILL.md) and [`qmltui-dev-loop`](../../.agents/skills/qmltui-dev-loop/SKILL.md) (`npx skills add qmltui/qmltui`) for QML conventions and closed-loop frame verification.

## CMake Packaging and Diagnostics

- **Automatic QML import paths, split export sets, and exported CLI**: `find_package(QmlTui 0.6)` configures `QT_QML_IMPORT_PATH` and `QML_IMPORT_PATH` automatically, sets relative `$ORIGIN` / `@loader_path` `INSTALL_RPATH` entries and `SOVERSION 0.6` on installed shared libraries, isolates optional `COMPONENTS` export sets, and exports `QmlTui::qmltui-runner`.
- **Live `AltScreen` logging**: Stream `console.log()`, `qDebug()`, and QML warnings unbuffered during alternate-screen sessions with `QMLTUI_LOG_FILE=<path>` or `2> debug.log`.
- **Documentation portal and showcase**: Interactive VitePress documentation and browser playground live in [`qmltui/qmltui.github.io`](https://github.com/qmltui/qmltui.github.io), alongside 13 full-scale applications in [`qmltui/apps`](https://github.com/qmltui/apps).

## Earlier Releases

- [Porting from 0.4 to 0.5](0.5.md)
- [Porting from 0.3 to 0.4](0.4.md)
- [Porting from 0.2 to 0.3](0.3.md)
- [0.1 Stabilization Notes](0.1.md)
