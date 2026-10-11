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
