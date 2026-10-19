# TerminalView

## Class Overview

`qmltui::TerminalView` is an embedded terminal emulator item that spawns a child process inside a Unix pseudo-terminal (PTY) via `UnixPtySession` and decodes its VT stream into the QmlTui cell grid using `libvterm`. It supports scrollback history, 16-color palette customization, keyboard and SGR mouse forwarding while focused, and opt-in OSC title, clipboard, and notification signals.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/TerminalHost/TerminalView.h` (installed as `<QmlTui/TerminalHost/TerminalView.h>`)
- **Implementation**: `src/QmlTui/TerminalHost/TerminalView.cpp`
- **CMake Target**: `qmltui::terminal_host` (`QmlTui::TerminalHost`)
- **Feature Gate**: Built and installed only on Linux and macOS when `QMLTUI_ENABLE_TERMINAL_HOST=ON`.
- **QML Import**: `import QmlTui.TerminalHost`

## Class Hierarchy and Role

- **Base Classes**: `qmltui::Item`, `QQmlParserStatus`
- **Role**: Visual item hosting an interactive child PTY session inside a QmlTui layout.

## Enumerations

### ProcessState

| Value | Description |
| :--- | :--- |
| `TerminalView::NotRunning` | No child process is running. |
| `TerminalView::Starting` | The PTY session and child process are being initialized. |
| `TerminalView::Running` | The child process is active inside the PTY. |
| `TerminalView::Stopping` | Termination has been requested (`terminate()` or `kill()`) and the process is shutting down. |

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `program` | `QString` | `program` | `setProgram` | `programChanged` | Executable path or command name to launch (falls back to `defaultUserShell()` when empty). |
| `arguments` | `QStringList` | `arguments` | `setArguments` | `argumentsChanged` | Arguments passed directly to `program` without shell expansion. |
| `workingDirectory` | `QString` | `workingDirectory` | `setWorkingDirectory` | `workingDirectoryChanged` | Initial working directory for the child process. |
| `environment` | `QVariantMap` | `environment` | `setEnvironment` | `environmentChanged` | Extra environment variables merged into the child environment. |
| `inheritEnvironment` | `bool` | `inheritEnvironment` | `setInheritEnvironment` | `inheritEnvironmentChanged` | Whether the child inherits the parent process environment (`true` by default). |
| `terminalType` | `QString` | `terminalType` | `setTerminalType` | `terminalTypeChanged` | Value assigned to the child's `TERM` variable (`"xterm-256color"` by default). |
| `autoStart` | `bool` | `autoStart` | `setAutoStart` | `autoStartChanged` | Starts the child automatically when `componentComplete()` runs (`false` by default). |
| `processState` | `ProcessState` | `processState` | — | `processStateChanged` | Current lifecycle state (`NotRunning`, `Starting`, `Running`, `Stopping`). |
| `running` | `bool` | `running` | — | `runningChanged` | True while `processState == Running`. |
| `processId` | `qint64` | `processId` | — | `processIdChanged` | OS process ID (PID) of the child process, or `-1` when not running. |
| `exitCode` | `int` | `exitCode` | — | `exitCodeChanged` | Exit status code of the last terminated child process (`-1` before exit). |
| `errorString` | `QString` | `errorString` | — | `errorStringChanged` | Diagnostic message from the last PTY or spawn error. |
| `title` | `QString` | `title` | — | `titleChanged` | Window title set by the child process via OSC 0/2 when `allowTitleChanges` is true. |
| `foregroundColor` | `qmltui::Color` | `foregroundColor` | `setForegroundColor` | `foregroundColorChanged` | Default foreground color (`"white"` by default). |
| `backgroundColor` | `qmltui::Color` | `backgroundColor` | `setBackgroundColor` | `backgroundColorChanged` | Default background color (`"black"` by default). |
| `palette` | `QList<qmltui::Color>` | `palette` | `setPalette` | `paletteChanged` | Optional 16-entry ANSI color palette override. |
| `scrollbackLimit` | `int` | `scrollbackLimit` | `setScrollbackLimit` | `scrollbackLimitChanged` | Maximum retained scrollback lines (`10000` by default). |
| `scrollOffset` | `int` | `scrollOffset` | `setScrollOffset` | `scrollOffsetChanged` | Rows scrolled back into history (`0` shows the live screen). |
| `scrollbackSize` | `int` | `scrollbackSize` | — | `scrollbackSizeChanged` | Current number of lines stored in the scrollback buffer. |
| `showScrollbar` | `bool` | `showScrollbar` | `setShowScrollbar` | `showScrollbarChanged` | Whether to draw a scrollbar thumb when scrollback history is present (`true` by default). |
| `allowTitleChanges` | `bool` | `allowTitleChanges` | `setAllowTitleChanges` | `allowTitleChangesChanged` | Allows child OSC title sequences to update `title` (`true` by default). |
| `allowClipboardWrite` | `bool` | `allowClipboardWrite` | `setAllowClipboardWrite` | `allowClipboardWriteChanged` | Allows child OSC 52 sequences to emit `clipboardWriteRequested()` (`false` by default). |
| `allowNotifications` | `bool` | `allowNotifications` | `setAllowNotifications` | `allowNotificationsChanged` | Allows child OSC 9/777 sequences to emit `notificationRequested()` (`false` by default). |

## Public and Q_INVOKABLE Methods

#### static QString defaultUserShell() [Q_INVOKABLE]
Resolves the current user's login shell from `$SHELL`, `getpwuid()`, or `/bin/sh`.

#### void start() [Q_INVOKABLE]
Spawns `program()` (or `defaultUserShell()` when `program()` is empty) in a fresh PTY sized to the item's cell geometry.

#### void terminate() [Q_INVOKABLE]
Sends `SIGTERM` to the child process group to request graceful shutdown.

#### void kill() [Q_INVOKABLE]
Sends `SIGKILL` to the child process group for immediate termination.

#### void sendText(const QString& text) [Q_INVOKABLE]
Writes UTF-8 `text` directly to the PTY master descriptor.

#### void clearScrollback() [Q_INVOKABLE]
Clears all retained scrollback lines and resets `scrollOffset` to `0`.

#### void scrollToBottom() [Q_INVOKABLE]
Resets `scrollOffset` to `0` so the live bottom viewport is shown.

## Signals

| Signal | Description |
| :--- | :--- |
| `started()` | Emitted when the child process has launched inside the PTY. |
| `finished(int exitCode)` | Emitted when the child process exits, providing its exit code. |
| `errorOccurred(const QString& error)` | Emitted when PTY creation or process launch fails. |
| `bell()` | Emitted when the child process writes `BEL` (`\a`). |
| `clipboardWriteRequested(const QString& text)` | Emitted when `allowClipboardWrite` is true and the child sends an OSC 52 write sequence. |
| `notificationRequested(const QString& title, const QString& message)` | Emitted when `allowNotifications` is true and the child sends an OSC 9 or OSC 777 notification. |

## Usage Example

Link against `qmltui::terminal_host` when `QMLTUI_ENABLE_TERMINAL_HOST` is enabled:

```cpp
#include <QmlTui/TerminalHost/TerminalView.h>

auto* view = new qmltui::TerminalView(parentItem);
view->setWidth(80);
view->setHeight(24);
view->setProgram(qmltui::TerminalView::defaultUserShell());
view->setArguments({QStringLiteral("-l")});
view->setFocus(true);
view->start();
```

See the [QML type reference](../qml-types/terminalview.md) for QML usage.
