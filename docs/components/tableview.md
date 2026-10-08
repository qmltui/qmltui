---
title: TableView
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::TableView
category: data views
summary: _TODO_
---

# TableView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `columns` | `list<var>` |  | _TODO_ |
| `model` | `var` |  | _TODO_ |
| `selectedRow` | `int` |  | _TODO_ |
| `currentRow` | `int` |  | Read-only. _TODO_ |
| `currentColumn` | `int` |  | _TODO_ |
| `alternatingRows` | `bool` |  | _TODO_ |
| `headerColor` | `rgba` |  | _TODO_ |
| `headerBackgroundColor` | `rgba` |  | _TODO_ |
| `selectionColor` | `rgba` |  | _TODO_ |
| `selectionBackgroundColor` | `rgba` |  | _TODO_ |
| `textColor` | `rgba` |  | _TODO_ |
| `rowBackgroundColor` | `rgba` |  | _TODO_ |
| `alternatingRowBackgroundColor` | `rgba` |  | _TODO_ |
| `showGrid` | `bool` |  | _TODO_ |
| `columnWidthProvider` | `var` |  | _TODO_ |
| `cellFormat` | `var` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `rowDelegate` | `Component` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `rowActivated(int index)` | _TODO_ |
| `headerClicked(int column)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void forceLayout()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    TableView {
        anchors.centerIn: parent
    }
}
```
