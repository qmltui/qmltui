---
title: Keys
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Keys
category: input events
summary: _TODO_
---

# Keys QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `priority` | `Priority` |  | _TODO_ |
| `forwardTo` | `list<Item>` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `pressed(KeyEvent event)` | _TODO_ |
| `returnPressed(KeyEvent event)` | _TODO_ |
| `escapePressed(KeyEvent event)` | _TODO_ |
| `spacePressed(KeyEvent event)` | _TODO_ |
| `upPressed(KeyEvent event)` | _TODO_ |
| `downPressed(KeyEvent event)` | _TODO_ |
| `leftPressed(KeyEvent event)` | _TODO_ |
| `rightPressed(KeyEvent event)` | _TODO_ |
| `tabPressed(KeyEvent event)` | _TODO_ |

## Enumeration Keys.Priority

| Value | Description |
| :--- | :--- |
| `Keys.BeforeItem` | _TODO_ |
| `Keys.AfterItem` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    // _TODO_ show Keys in use
}
```
