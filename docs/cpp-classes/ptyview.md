# PtyView

`qmltui::PtyView` is an embedded terminal emulator item that spawns a child process inside a Unix pseudo-terminal (PTY) and decodes its VT output stream into the QmlTui cell grid using `libvterm`. It supports scrollback history, 16-color palette overrides, keyboard and SGR mouse forwarding while focused, and opt-in OSC title, clipboard, and notification signals.

- **Header**: `<QmlTui/PtyView/PtyView.h>`
- **Inherits**: `qmltui::Item`, `QQmlParserStatus`
- **Target**: `QmlTui::PtyView` (`QMLTUI_ENABLE_PTYVIEW=ON`, Linux and macOS)
- **QML Import**: `import QmlTui.PtyView`

## `ProcessState`

| Value | Description |
| :--- | :--- |
| `PtyView::NotRunning` | No child process is running. |
| `PtyView::Starting` | The PTY session and child process are initializing. |
| `PtyView::Running` | The child process is active inside the PTY. |
| `PtyView::Stopping` | Termination has been requested (`terminate()` or `kill()`). |

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `program` | `QString` | read/write | `programChanged` | Executable path or name (falls back to `defaultUserShell()` when empty). |
| `arguments` | `QStringList` | read/write | `argumentsChanged` | Arguments passed directly via `QProcess::start()` without shell expansion (when `arguments` is empty and `program` is not found on disk or `PATH`, `start()` falls back to `QProcess::splitCommand(program)` and populates `arguments` from the remaining tokens). |
| `workingDirectory` | `QString` | read/write | `workingDirectoryChanged` | Initial working directory for the child process. |
| `environment` | `QVariantMap` | read/write | `environmentChanged` | Extra environment variables merged into the child environment. |
| `inheritEnvironment` | `bool` | read/write | `inheritEnvironmentChanged` | Whether the child inherits the parent environment (`true` by default). |
| `terminalType` | `QString` | read/write | `terminalTypeChanged` | Value assigned to `TERM` (`"xterm-256color"` by default). |
| `autoStart` | `bool` | read/write | `autoStartChanged` | Starts the process automatically on `componentComplete()` (`false` by default). |
| `processState` | `ProcessState` | read-only | `processStateChanged` | Current lifecycle state. |
| `running` | `bool` | read-only | `runningChanged` | `true` while `processState == Running`. |
| `processId` | `qint64` | read-only | `processIdChanged` | OS process ID (PID) of the child, or `0` when not running (matching `QProcess::processId()`). |
| `exitCode` | `int` | read-only | `exitCodeChanged` | Exit status of the last terminated child process (`-1` before exit). |
| `errorString` | `QString` | read-only | `errorStringChanged` | Diagnostic message from the last PTY or spawn error. |
| `title` | `QString` | read-only | `titleChanged` | Window title set via OSC 0/2 when `allowTitleChanges` is `true`. |
| `textColor` | `qmltui::Color` | read/write | `textColorChanged` | Default foreground text color (terminal default ink when unset). |
| `backgroundColor` | `qmltui::Color` | read/write | `backgroundColorChanged` | Default background color (`Color::Transparent` by default, preserving parent fills). |
| `palette` | `QList<qmltui::Color>` | read/write | `paletteChanged` | Optional 16-entry ANSI color palette override. |
| `scrollbackLimit` | `int` | read/write | `scrollbackLimitChanged` | Maximum retained scrollback lines (`10000` by default). |
| `scrollOffset` | `int` | read/write | `scrollOffsetChanged` | Rows scrolled back into history (`0` shows the live screen). |
| `scrollbackSize` | `int` | read-only | `scrollbackSizeChanged` | Number of lines currently stored in scrollback. |
| `scrollBarVisible` | `bool` | read/write | `scrollBarVisibleChanged` | Draws a scrollbar thumb when scrollback history is present (`true` by default). |
| `allowTitleChanges` | `bool` | read/write | `allowTitleChangesChanged` | Allows child OSC 0/2 sequences to update `title` (`true` by default). |
| `allowClipboardWrite` | `bool` | read/write | `allowClipboardWriteChanged` | Allows child OSC 52 sequences to emit `clipboardWriteRequested()` (`false` by default). |
| `allowNotifications` | `bool` | read/write | `allowNotificationsChanged` | Allows child OSC 9/777 sequences to emit `notificationRequested()` (`false` by default). |

## Public and `Q_INVOKABLE` Methods

- `static QString defaultUserShell()` — resolves the current user's login shell from `$SHELL`, `getpwuid()`, or `/bin/sh`.
- `void start()` — spawns `program()` (or `defaultUserShell()`) in a PTY sized to the item's cell geometry.
- `void terminate()` — sends `SIGTERM` to the child process group.
- `void kill()` — sends `SIGKILL` to the child process group.
- `void sendText(const QString &text)` — writes UTF-8 `text` to the PTY master descriptor.
- `void clearScrollback()` — clears retained scrollback lines and resets `scrollOffset` to `0`.
- `void scrollToBottom()` — resets `scrollOffset` to `0`.

## Signals

| Signal | Description |
| :--- | :--- |
| `started()` | Emitted when the child process has launched. |
| `finished(int exitCode)` | Emitted when the child process exits. |
| `errorOccurred(const QString &error)` | Emitted when PTY creation or process launch fails. |
| `bell()` | Emitted when the child writes `BEL` (`\a`). |
| `clipboardWriteRequested(const QString &text)` | Emitted when `allowClipboardWrite` is `true` and the child sends an OSC 52 write sequence. |
| `notificationRequested(const QString &title, const QString &message)` | Emitted when `allowNotifications` is `true` and the child sends an OSC 9 or OSC 777 notification. |

## Example

```cpp
#include <QmlTui/PtyView/PtyView.h>

auto *view = new qmltui::PtyView(parentItem);
view->setWidth(80);
view->setHeight(24);
view->setProgram(qmltui::PtyView::defaultUserShell());
view->setArguments({QStringLiteral("-l")});
view->setFocus(true);
view->start();
```

See [Embedded PTY Shells](../guides/pty-view.md) and the [PtyView QML type reference](../qml-types/ptyview.md).
