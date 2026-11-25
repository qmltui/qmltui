# Terminal

`qmltui::Terminal` is the root visual item and session coordinator for a QmlTui application. It initializes and owns the `TerminalEngine`, tracks host terminal dimensions, manages active keyboard focus, pointer hover state, and modal `Popup` overlays, and selects between full-screen (`Mode::AltScreen`) and inline (`Mode::Inline`) rendering.

`Terminal` does not bind `Q` or `Escape` to application exit; applications define their own keyboard quit handler.

- **Header**: `<QmlTui/Core/Items/Terminal.h>`
- **Inherits**: `qmltui::Item`, `QQmlParserStatus`
- **Target**: `QmlTui::Core`

## Enumerations

### `Mode`

| Value | Description |
| :--- | :--- |
| `Mode::AltScreen` | Switches to the terminal's alternate screen buffer and renders across the full viewport. |
| `Mode::Inline` | Renders within the normal screen buffer using content-driven height while preserving scrollback. |

### `ClipboardMode`

| Value | Description |
| :--- | :--- |
| `ClipboardMode::Disabled` | Suppresses OSC 52 escape sequences while still emitting `clipboardCopyRequested()`. |
| `ClipboardMode::Osc52` | Enables OSC 52 clipboard writes and asynchronous reads via `Clipboard`. |

### `ProgressState`

| Value | Integer | Description |
| :--- | :--- | :--- |
| `ProgressNone` | `0` | Hides the OSC 9;4 terminal progress indicator. |
| `ProgressNormal` | `1` | Reports normal progress using `progress()`. |
| `ProgressError` | `2` | Reports error progress state using `progress()`. |
| `ProgressIndeterminate` | `3` | Displays an indeterminate progress indicator. |
| `ProgressWarning` | `4` | Reports paused or warning progress state using `progress()`. |

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `capabilities` | `qmltui::TerminalCapabilities` | read-only | `capabilitiesChanged` | Detected or overridden host terminal capabilities. |
| `title` | `QString` | read/write | `titleChanged` | Host terminal window title emitted via OSC 0 (`\033]0;...\a`). |
| `color` | `qmltui::Color` | read/write | `colorChanged` | Root background color (`"transparent"` by default). |
| `mode` | `Mode` | read/write | `modeChanged` | Presentation mode (`Mode::AltScreen` or `Mode::Inline`). |
| `clipboardMode` | `ClipboardMode` | read/write | `clipboardModeChanged` | Clipboard mode (`ClipboardMode::Osc52` by default). |
| `mouseReporting` | `bool` | read/write | `mouseReportingChanged` | Enables SGR mouse tracking (`?1000h`/`?1002h`/`?1006h`); any-motion (`?1003h`) is added while items request hover. |
| `hoveredItem` | `qmltui::Item*` | read-only | `hoveredItemChanged` | Item currently under the mouse pointer. |
| `terminalWidth`, `terminalHeight` | `int` | read-only | `terminalSizeChanged` | Host terminal viewport width and height in cells. |
| `applicationActive` | `bool` | read-only | `applicationActiveChanged` | Host terminal focus state reported via DEC mode 1004 (`?1004h`). |
| `progress` | `qreal` | read/write | `progressChanged` | Progress value in `[0.0, 1.0]` for OSC 9;4 reporting. |
| `progressState` | `ProgressState` | read/write | `progressStateChanged` | Progress indicator state for OSC 9;4 reporting. |

In addition to the property notify signals above, `Terminal` emits `clipboardCopyRequested(const QString &text)` on every `copyToClipboard()` call, `focusedItemChanged()` when active focus moves, and `mouseMotionRequestedChanged(bool requested)` when passive hover tracking toggles.

## Public Methods

- `TerminalEngine *engine() const` — returns the `TerminalEngine` for this session, creating it on demand if `componentComplete()` has not run yet. In installed `find_package(QmlTui)` builds, `TerminalEngine` is forward-declared only (an opaque pointer — [`TerminalEngine.h`](terminalengine.md) is an in-tree internal header), so installed C++ consumers should call `engine()` only as an eager-initialization trigger and drive the session through `Terminal`, `Scrollback`, and `Runner::startFromEnvironment()`.
- `bool enqueueControlSequence(QByteArray bytes)` / `QByteArray takeControlSequences()` — queues or drains terminal resource commands emitted before the next cell diff (bounded to 32 MiB per frame).
- `bool enqueuePostControlSequence(QByteArray bytes)` / `QByteArray takePostControlSequences()` — queues or drains terminal resource commands (such as Sixel graphics) emitted after the cell diff each frame.
- `void copyToClipboard(const QString &text)` (`Q_INVOKABLE`) — emits `clipboardCopyRequested(text)` and, when `clipboardMode() == ClipboardMode::Osc52`, writes an OSC 52 sequence.
- `void bell()` (`Q_INVOKABLE`) — emits a terminal bell (`\x07`).
- `void queryColors()` (`Q_INVOKABLE`) — re-queries the host terminal's default foreground (`OSC 10`), default background (`OSC 11`), and 16 ANSI palette colors (`OSC 4;0..15`), emitting `capabilitiesChanged()` when replies update `capabilities()`. Also triggered automatically on DEC mode `2031` palette-change notifications (`CSI ? 997 ; 1 n` / `CSI ? 997 ; 2 n`).
- `void setApplicationActive(bool active)` — updates the host terminal window focus state (`DEC mode 1004`; `applicationActive` is read-only from QML).
- `Item *focusedItem() const` / `void setFocusedItem(Item *item)` — returns or changes the item holding active keyboard focus.
- `void setHoveredItem(Item *item)` — updates the item under the mouse pointer (`hoveredItem` is read-only from QML).
- `void setMouseMotionRequested(QObject *requester, bool requested)` — registers or unregisters `requester` as needing passive any-motion mouse tracking (`?1003h`).
- `static void registerRunnerHook(RunnerHook hook)` — installs the startup hook used when `QmlTui::Runner` is linked.

## C++ Example

```cpp
#include <QCoreApplication>
#include <QmlTui/Core/Engine/Shortcut.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Text.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qmltui::Terminal terminal;
    terminal.setTitle(QStringLiteral("C++ Terminal App"));

    auto *label = new qmltui::Text(&terminal);
    label->setText(QStringLiteral("Hello from C++ — press Ctrl+Q to quit"));
    label->setColor(qmltui::Color(QStringLiteral("cyan")));

    auto *quitShortcut = new qmltui::Shortcut(&terminal);
    quitShortcut->setSequence(QStringLiteral("Ctrl+Q"));
    QObject::connect(quitShortcut, &qmltui::Shortcut::activated, &app, &QCoreApplication::quit);

    terminal.componentComplete();
    return app.exec();
}
```

See the [Terminal QML type reference](../qml-types/terminal.md) and [`examples/recipes/cpp_only/`](../../examples/recipes/cpp_only/).
