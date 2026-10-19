# Terminal

## Class Overview

`qmltui::Terminal` is the root visual item and session coordinator for a QmlTui application. It owns the `TerminalEngine`, tracks host terminal dimensions, manages active keyboard focus, pointer hover state, and modal `Popup` / `InputScope` overlays, and selects between full-viewport alternate-screen rendering (`Mode::AltScreen`) and inline scrollback stream rendering (`Mode::Inline`).

`Terminal` does not bind `Q` or `Escape` to quit automatically; applications define their own keyboard quit path.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Items/Terminal.h` (installed as `<QmlTui/Core/Items/Terminal.h>`)
- **Implementation**: `src/QmlTui/Core/Items/Terminal.cpp`
- **CMake Target**: `qmltui::core` (`QmlTui::Core`)
- **Dependencies**: `Item.h`, `TerminalCapabilities.h`, `QQmlParserStatus`.

## Class Hierarchy and Role

- **Base Classes**: `qmltui::Item`, `QQmlParserStatus`
- **Role**: Required root element of a QmlTui visual tree. Initializes its `TerminalEngine` on `componentComplete()` (when loaded via QML) or on first `engine()` access in C++-only applications.

## Enumerations

### Mode

| Value | Description |
| :--- | :--- |
| `Mode::AltScreen` | Switches to the terminal's alternate screen buffer and renders across the full terminal viewport. |
| `Mode::Inline` | Renders within the normal terminal buffer using content-driven height while preserving scrollback. |

### ClipboardMode

| Value | Description |
| :--- | :--- |
| `ClipboardMode::Disabled` | Suppresses OSC 52 terminal clipboard escape sequences while still emitting `clipboardCopyRequested()`. |
| `ClipboardMode::Osc52` | Enables OSC 52 clipboard writes and asynchronous reads via `Clipboard`. |

### ProgressState

| Value | Integer | Description |
| :--- | :--- | :--- |
| `ProgressNone` | `0` | Hides the OSC 9;4 terminal progress indicator. |
| `ProgressNormal` | `1` | Reports normal progress using `progress()`. |
| `ProgressError` | `2` | Reports error progress state using `progress()`. |
| `ProgressIndeterminate` | `3` | Displays an indeterminate progress indicator. |
| `ProgressWarning` | `4` | Reports paused or warning progress state using `progress()`. |

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `capabilities` | `qmltui::TerminalCapabilities` | `capabilities` | — | `capabilitiesChanged` | Detected or overridden host terminal capabilities. |
| `title` | `QString` | `title` | `setTitle` | `titleChanged` | Host terminal window title emitted via OSC 2. |
| `color` | `qmltui::Color` | `color` | `setColor` | `colorChanged` | Root background color (defaults to `"transparent"`). |
| `mode` | `Mode` | `mode` | `setMode` | `modeChanged` | Display mode (`Mode::AltScreen` or `Mode::Inline`). |
| `clipboardMode` | `ClipboardMode` | `clipboardMode` | `setClipboardMode` | `clipboardModeChanged` | Clipboard integration mode (`ClipboardMode::Osc52` by default). |
| `mouseReporting` | `bool` | `mouseReporting` | `setMouseReporting` | `mouseReportingChanged` | Enables SGR mouse tracking (`?1000h`/`?1002h`/`?1006h`); any-motion (`?1003h`) is added while items request hover. |
| `hoveredItem` | `qmltui::Item*` | `hoveredItem` | — | `hoveredItemChanged` | Item currently under the mouse pointer. |
| `terminalWidth` | `int` | `terminalWidth` | — | `terminalSizeChanged` | Host terminal viewport width in columns. |
| `terminalHeight` | `int` | `terminalHeight` | — | `terminalSizeChanged` | Host terminal viewport height in rows. |
| `applicationActive` | `bool` | `applicationActive` | — | `applicationActiveChanged` | Host terminal window focus state reported via DEC mode 1004 (`?1004h`). |
| `progress` | `qreal` | `progress` | `setProgress` | `progressChanged` | Progress value in `[0.0, 1.0]` for OSC 9;4 terminal progress reporting. |
| `progressState` | `ProgressState` | `progressState` | `setProgressState` | `progressStateChanged` | Progress indicator state for OSC 9;4 reporting. |

## Public Methods

#### explicit Terminal(QObject* parent = nullptr)
Constructs a root `Terminal` item.

#### TerminalEngine* engine() const
Returns the `TerminalEngine` coordinating I/O and rendering for this terminal, creating it on demand if `componentComplete()` has not run yet.

#### TerminalCapabilities capabilities() const
Returns the active `TerminalCapabilities` snapshot from the engine.

#### void copyToClipboard(const QString& text) [Q_INVOKABLE]
Emits `clipboardCopyRequested(text)` and, when `clipboardMode()` is `ClipboardMode::Osc52`, sends an OSC 52 clipboard write sequence through the engine.

#### void bell() [Q_INVOKABLE]
Emits a terminal bell (`\x07`) through the engine.

#### Item* focusedItem() const / void setFocusedItem(Item* item)
Returns or updates the item holding active keyboard focus within this terminal.

#### Item* hoveredItem() const / void setHoveredItem(Item* item)
Returns or updates the item currently under the mouse pointer.

#### void setMouseMotionRequested(QObject* requester, bool requested)
Registers or unregisters `requester` as needing passive any-motion mouse tracking (`?1003h`). Automatically cleaned up if `requester` is destroyed.

#### static void registerRunnerHook(RunnerHook hook)
Installs the startup hook invoked by `componentComplete()` when `qmltui::runner` is linked into the executable.

## Signals

| Signal | Description |
| :--- | :--- |
| `capabilitiesChanged()` | Emitted when detected or overridden terminal capabilities change. |
| `titleChanged()` | Emitted when `title` changes. |
| `colorChanged()` | Emitted when `color` changes. |
| `modeChanged()` | Emitted when `mode` changes between `AltScreen` and `Inline`. |
| `clipboardModeChanged()` | Emitted when `clipboardMode` changes. |
| `mouseReportingChanged()` | Emitted when `mouseReporting` changes. |
| `clipboardCopyRequested(const QString& text)` | Emitted on every `copyToClipboard()` call regardless of `clipboardMode`. |
| `focusedItemChanged()` | Emitted when the active focused item changes. |
| `hoveredItemChanged()` | Emitted when the item under the mouse pointer changes. |
| `terminalSizeChanged()` | Emitted when the host terminal column or row count changes. |
| `mouseMotionRequestedChanged(bool requested)` | Emitted when passive mouse-motion tracking requests transition between empty and non-empty. |
| `applicationActiveChanged()` | Emitted when the host terminal window gains or loses focus. |
| `progressChanged()` | Emitted when `progress` changes. |
| `progressStateChanged()` | Emitted when `progressState` changes. |

## Usage Example

```cpp
#include <QCoreApplication>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Text.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qmltui::Terminal terminal;
    terminal.setTitle(QStringLiteral("C++ Terminal App"));
    terminal.setColor(qmltui::Color(QStringLiteral("#0f172a")));

    auto* label = new qmltui::Text(&terminal);
    label->setText(QStringLiteral("Hello from C++"));
    label->setColor(qmltui::Color(QStringLiteral("cyan")));

    terminal.componentComplete();
    return app.exec();
}
```

See the [QML type reference](../qml-types/terminal.md) for QML usage.
