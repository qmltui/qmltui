---
title: TerminalView
kind: qml
import: Qamui.TerminalHost
since: "0.4"
inherits: Item
cpp: qamui::TerminalView
category: display
summary: _TODO_
---

# TerminalView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui.TerminalHost` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `program` | `string` |  | _TODO_ |
| `arguments` | `list<string>` |  | _TODO_ |
| `workingDirectory` | `string` |  | _TODO_ |
| `environment` | `var` |  | _TODO_ |
| `inheritEnvironment` | `bool` |  | _TODO_ |
| `terminalType` | `string` |  | _TODO_ |
| `autoStart` | `bool` |  | _TODO_ |
| `processState` | `ProcessState` |  | Read-only. _TODO_ |
| `running` | `bool` |  | Read-only. _TODO_ |
| `processId` | `qlonglong` |  | Read-only. _TODO_ |
| `exitCode` | `int` |  | Read-only. _TODO_ |
| `errorString` | `string` |  | Read-only. _TODO_ |
| `title` | `string` |  | Read-only. _TODO_ |
| `foregroundColor` | `rgba` |  | _TODO_ |
| `backgroundColor` | `rgba` |  | _TODO_ |
| `palette` | `list<rgba>` |  | _TODO_ |
| `scrollbackLimit` | `int` |  | _TODO_ |
| `scrollOffset` | `int` |  | _TODO_ |
| `scrollbackSize` | `int` |  | Read-only. _TODO_ |
| `showScrollbar` | `bool` |  | _TODO_ |
| `allowTitleChanges` | `bool` |  | _TODO_ |
| `allowClipboardWrite` | `bool` |  | _TODO_ |
| `allowNotifications` | `bool` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `started()` | _TODO_ |
| `finished(int exitCode)` | _TODO_ |
| `errorOccurred(string error)` | _TODO_ |
| `bell()` | _TODO_ |
| `clipboardWriteRequested(string text)` | _TODO_ |
| `notificationRequested(string title, string message)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `string defaultUserShell()` | _TODO_ |
| `void start()` | _TODO_ |
| `void terminate()` | _TODO_ |
| `void kill()` | _TODO_ |
| `void sendText(string text)` | _TODO_ |
| `void clearScrollback()` | _TODO_ |
| `void scrollToBottom()` | _TODO_ |

## Enumeration TerminalView.ProcessState

| Value | Description |
| :--- | :--- |
| `TerminalView.NotRunning` | _TODO_ |
| `TerminalView.Starting` | _TODO_ |
| `TerminalView.Running` | _TODO_ |
| `TerminalView.Stopping` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui
import Qamui.TerminalHost

Terminal {
    // _TODO_ show TerminalView in use
}
```

## Hosting a terminal process

`TerminalView` is an experimental QML item that hosts an external terminal
application in a Unix pseudo-terminal. It is available only when Qamui is built
with `QAMUI_ENABLE_TERMINAL_HOST=ON` and currently supports Linux and macOS.

```cmake
set(QAMUI_ENABLE_TERMINAL_HOST ON)
add_subdirectory(qamui)
target_link_libraries(my_app PRIVATE Qamui::TerminalHost)
```

```qml
// Inside a Terminal importing Qamui.TerminalHost:
TerminalView {
    width: 80
    height: 24
    focus: true
    program: "/bin/sh"
    arguments: ["-l"]
    autoStart: true
}
```

Programs and arguments are passed directly to the child process. Qamui never
implicitly invokes a shell or expands shell syntax. The child receives
`TERM=xterm-256color`, `COLORTERM=truecolor`, and `TERM_PROGRAM=qamui` unless a
different `terminalType` is selected.

The process lifecycle is exposed through `processState`, `running`,
`processId`, `exitCode`, and `errorString`. The name `processState` is used
because every Qamui `Item` already has a `state` property for UI states.

`scrollOffset` is zero at the live viewport and increases into bounded history.
Mouse wheel input scrolls locally unless the child has enabled mouse reporting;
Shift+wheel always scrolls history. Text selection and copy UI are not provided
in this first version.

Title changes are enabled by default. OSC 52 clipboard writes and notification
requests are disabled by default and, when enabled, are emitted as signals for
the host application to handle. TerminalView never changes the system clipboard
or creates an operating-system notification directly.

Qamui uses unmodified libvterm 0.3.3. libvterm does not expose SGR 2 faint/dim,
so faint text is currently rendered at normal intensity. Image protocols,
clickable OSC 8 links, terminal cursor shapes, and Windows ConPTY are also out
of scope for this version.

For offline builds, set `FETCHCONTENT_SOURCE_DIR_LIBVTERM` to an extracted
libvterm 0.3.3 release archive. The archive hash pinned by Qamui is checked for
normal FetchContent downloads.

## Multi-Terminal Setups & Data Interchange

Applications can host multiple `TerminalView` instances in a split layout and interchange data between them programmatically using `sendText(text)`:

```qml
// Send text or commands from Terminal 1 context into Terminal 2
terminal2.sendText("echo 'Payload from Terminal 1'\n")

// Broadcast command input to multiple terminals simultaneously
terminal1.sendText(commandInput.text + "\n")
terminal2.sendText(commandInput.text + "\n")
```

The `qamui_terminal_view_demo` component example demonstrates a full dual-terminal workspace with focus switching (`F1`/`F2`/`F3`), lifecycle controls (start/stop/restart/clear), direct inter-terminal payload transfer, and a command dispatcher bar.
