# Changes in 0.5.0

The framework and QML modules use the QmlTui name. Environment settings use
`QMLTUI_*`, including Runner's `QMLTUI_RUNNER_*` variables. Frame logging uses
`qmltui.frames`.

## Migrating from 0.4 to 0.5

| 0.4 API / Option | 0.5 Replacement |
| :--- | :--- |
| `TestRunner` (`qmltui::test_runner`, `QMLTUI_TEST_*`) | [`Runner`](../cpp-classes/runner.md) (`qmltui::runner`, `qmltui-runner`, `QMLTUI_RUNNER_*`) |
| `-DQMLTUI_ENABLE_BEHAVIOR=ON/OFF` | `-DQMLTUI_ENABLE_PRIVATE_QT=ON/OFF` (controls `Behavior` and private-Qt `PropertyChanges` binding restoration) |
| `TerminalHost` / `TerminalView` (`import QmlTui.TerminalView`) | [`PtyView`](../qml-types/ptyview.md) (`import QmlTui.PtyView`, `textColor`, `backgroundColor`, `scrollBarVisible`) |
| `Terminal.FullScreen` | `Terminal.AltScreen` (with `Terminal.Inline` for normal-buffer CLI tools) |
| `Scrollback.write(text)` | [`Scrollback.print(text)`](../qml-types/scrollback.md) and `Scrollback.append(...)` |
| `Shortcut.sequence` integer bitmask only | [`Shortcut.sequence`](../qml-types/shortcut.md) accepts both key-chord strings (`"Ctrl+Q"`, `"F5"`) and Qt bitmasks |
| `Animation` finite loop counts only | [`Animation.Infinite`](../qml-types/animation.md) (`-1`) for continuous animations |
| `Palette`, `Switch`, `Toast` in `QmlTui.Controls` | Per-item color properties (`color`, `textColor`, `borderColor`) and recipes under `examples/recipes/` |

## Highlights

The gallery now lives under `examples/gallery`; standalone recipes and guide
examples live under `examples/recipes`. Example targets consistently use
`qmltui_<directory>_demo`. Full applications moved to the separate qmltui-apps
project.

`find_package(QmlTui)` now populates `QT_QML_IMPORT_PATH` automatically for
`qt_add_qml_module()` consumers, and applications can stream `console.log()` /
`qDebug()` output live during `AltScreen` sessions via `QMLTUI_LOG_FILE=<path>`
or `2> debug.log`.

Documentation now has Getting Started, Guides, QML Types, C++ Classes,
Examples, and Changes entry points. Reference documentation no longer lives
beside implementation files.

Runner supports committed-scrollback assertions, deterministic virtual time,
extended `expectStyle()` attribute checks (`blink`, `conceal`, `strikethrough`,
with unknown-key rejection), and asciicast v2 recording with `RECORD`,
`startRecording()`, `stopRecording()`, and `marker()`. See the [Runner API](../cpp-classes/runner.md).

- [Porting from 0.1](0.1.md)
- [Porting from 0.3](0.3.md)
- [Porting from 0.4](0.4.md)
