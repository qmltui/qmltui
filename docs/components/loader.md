---
title: Loader
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::Loader
category: layout
summary: _TODO_
---

# Loader QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `active` | `bool` |  | _TODO_ |
| `source` | `url` |  | _TODO_ |
| `sourceComponent` | `Component` |  | _TODO_ |
| `item` | `Item` |  | Read-only. _TODO_ |
| `status` | `Status` |  | Read-only. _TODO_ |
| `progress` | `real` |  | Read-only. _TODO_ |
| `asynchronous` | `bool` |  | _TODO_ |
| `errorString` | `string` |  | Read-only. _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `loaded()` | _TODO_ |

## Enumeration Loader.Status

| Value | Description |
| :--- | :--- |
| `Loader.Null` | _TODO_ |
| `Loader.Ready` | _TODO_ |
| `Loader.Loading` | _TODO_ |
| `Loader.Error` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    Loader {
        anchors.centerIn: parent
    }
}
```
