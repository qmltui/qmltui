---
title: PtyView
kind: qml
import: QmlTui.PtyView
since: "0.4"
inherits: Item
cpp: qmltui::PtyView
category: display
summary: Embedded terminal emulator hosting a process inside a pseudo-terminal (PTY).
---

# PtyView QML type

Embedded terminal emulator hosting a process inside a pseudo-terminal (PTY). [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.PtyView` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `program` | `string` | `""` | Holds the executable binary path or command name to execute in the PTY. |
| `arguments` | `list<string>` | `[]` | Holds the list of command-line arguments passed to `program`. |
| `workingDirectory` | `string` | `""` | Holds the initial working directory path for the child process. |
| `environment` | `var` | `{}` | Holds additional custom environment variables for the child process. |
| `inheritEnvironment` | `bool` | `true` | Whether the child process inherits environment variables from the parent process. |
| `terminalType` | `string` | `"xterm-256color"` | Holds the `TERM` environment variable reported to the child process. |
| `autoStart` | `bool` | `false` | Whether the process starts automatically when the component completes. |
| `processState` | `ProcessState` |  | Read-only. Holds the current process lifecycle state: `NotRunning`, `Starting`, `Running`, or `Stopping`. |
| `running` | `bool` |  | Read-only. Holds whether the child process is currently active and running. |
| `processId` | `int` |  | Read-only. Holds the operating system process ID (PID) of the child process. |
| `exitCode` | `int` |  | Read-only. Holds the exit status code after the process terminates. |
| `errorString` | `string` |  | Read-only. Holds error details if process spawning or execution fails. |
| `title` | `string` |  | Read-only. Holds the dynamic terminal title requested by the child process via escape sequences. |
| `foregroundColor` | `color` | `"white"` | Holds the default foreground text color. |
| `backgroundColor` | `color` | `"black"` | Holds the default background color. |
| `palette` | `list<color>` | `[]` | Holds the 16 standard ANSI colors used by the terminal emulator. |
| `scrollbackLimit` | `int` | `10000` | Holds the maximum number of scrollback lines retained in history. |
| `scrollOffset` | `int` | `0` | Holds the current scroll offset into scrollback history (0 at live screen). |
| `scrollbackSize` | `int` |  | Read-only. Holds the current line count in the scrollback buffer. |
| `showScrollbar` | `bool` | `true` | Whether to display a vertical scrollbar when scrollback history exists. |
| `allowTitleChanges` | `bool` | `true` | Whether child process title changes via OSC escape sequences update `title`. |
| `allowClipboardWrite` | `bool` | `false` | Whether child process OSC 52 clipboard write requests emit `clipboardWriteRequested`. |
| `allowNotifications` | `bool` | `false` | Whether child process OSC notification requests emit `notificationRequested`. |

## Signals

| Signal | Description |
| :--- | :--- |
| `started()` | Emitted when the child process has launched successfully. |
| `finished(int exitCode)` | Emitted when the child process terminates, providing its exit code. |
| `errorOccurred(string error)` | Emitted when a PTY or process lifecycle error occurs. |
| `bell()` | Emitted when the child process triggers a terminal bell (`\a`). |
| `clipboardWriteRequested(string text)` | Emitted when the child requests clipboard modification via OSC 52. |
| `notificationRequested(string title, string message)` | Emitted when the child requests an operating system desktop notification. |

## Methods

| Method | Description |
| :--- | :--- |
| `string defaultUserShell()` | Returns the default login shell path for the current user account. |
| `void start()` | Launches the configured child process. |
| `void terminate()` | Sends SIGTERM to request graceful process termination. |
| `void kill()` | Sends SIGKILL to force immediate process termination. |
| `void sendText(string text)` | Writes text or escape sequences directly into the PTY input stream. |
| `void clearScrollback()` | Clears all retained lines from the scrollback history buffer. |
| `void scrollToBottom()` | Resets `scrollOffset` to 0 to view the live cursor line. |

## Enumeration PtyView.ProcessState

| Value | Description |
| :--- | :--- |
| `PtyView.NotRunning` | The process is inactive and not running. |
| `PtyView.Starting` | The process is currently launching and initializing its PTY. |
| `PtyView.Running` | The process is active and running in the PTY. |
| `PtyView.Stopping` | The process has been signaled and is shutting down. |

## Detailed Description

PtyView is a QML item that hosts an external terminal application inside
a Unix pseudo-terminal (PTY) using libvterm. It is available when QmlTui is built
with `QMLTUI_ENABLE_PTYVIEW=ON` (supported on Linux and macOS).

Programs and arguments are passed directly to the child process. QmlTui never
implicitly invokes a shell or expands shell syntax. The child receives
`TERM=xterm-256color`, `COLORTERM=truecolor`, and `TERM_PROGRAM=qmltui` unless a
different `terminalType` is selected.

The process lifecycle is exposed through `processState`, `running`, `processId`,
`exitCode`, and `errorString`. Applications can host multiple PtyView instances
in a split layout and interchange data between them programmatically using `sendText(text)`.

### Example

```qml
import QtQml
import QmlTui
import QmlTui.PtyView

Terminal {
    Keys.onPressed: event => {
        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    PtyView {
        anchors.fill: parent
        focus: true
        program: "/bin/sh"
        arguments: ["-l"]
        autoStart: true
        onFinished: Qt.quit()
    }
}
```

### Gallery Example

See [`examples/gallery/examples/PtyView.qml`](../../examples/gallery/examples/PtyView.qml) (`./build/examples/gallery/qmltui_gallery --example PtyView`, or open in the [WebAssembly Playground](../playground.md?example=PtyView.qml)).
