---
title: Terminal
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::Terminal
category: core
summary: _TODO_
---

# Terminal QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `capabilities` | `terminalCapabilities` |  | Read-only. _TODO_ |
| `title` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `mode` | `Mode` |  | _TODO_ |
| `clipboardMode` | `ClipboardMode` |  | _TODO_ |
| `mouseReporting` | `bool` |  | _TODO_ |
| `hoveredItem` | `Item` |  | Read-only. _TODO_ |
| `terminalWidth` | `int` |  | Read-only. _TODO_ |
| `terminalHeight` | `int` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `clipboardCopyRequested(string text)` | _TODO_ |
| `focusedItemChanged()` | _TODO_ |
| `mouseMotionRequestedChanged(bool requested)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void copyToClipboard(string text)` | _TODO_ |

## Enumeration Terminal.Mode

| Value | Description |
| :--- | :--- |
| `Terminal.FullScreen` | _TODO_ |
| `Terminal.Inline` | _TODO_ |

## Enumeration Terminal.ClipboardMode

| Value | Description |
| :--- | :--- |
| `Terminal.Disabled` | _TODO_ |
| `Terminal.Osc52` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    Text { text: "Hello" }
}
```
