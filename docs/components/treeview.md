---
title: TreeView
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::TreeView
category: data views
summary: _TODO_
---

# TreeView QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | _TODO_ |
| `rootNode` | `var` |  | _TODO_ |
| `rootIndex` | `QModelIndex` |  | _TODO_ |
| `delegate` | `Component` |  | _TODO_ |
| `currentIndex` | `int` |  | _TODO_ |
| `selectedIndex` | `int` |  | _TODO_ |
| `currentRow` | `int` |  | Read-only. _TODO_ |
| `currentColumn` | `int` |  | Read-only. _TODO_ |
| `currentItemData` | `var` |  | Read-only. _TODO_ |
| `selectionColor` | `rgba` |  | _TODO_ |
| `selectionBackgroundColor` | `rgba` |  | _TODO_ |
| `textColor` | `rgba` |  | _TODO_ |
| `branchColor` | `rgba` |  | _TODO_ |
| `expandedIcon` | `string` |  | _TODO_ |
| `collapsedIcon` | `string` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `nodeActivated(string path)` | _TODO_ |
| `itemActivated(int index)` | _TODO_ |
| `expanded(int row, int depth)` | _TODO_ |
| `collapsed(int row, bool recursively)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void expand(int index)` | _TODO_ |
| `void collapse(int index)` | _TODO_ |
| `void toggleExpanded(int index)` | _TODO_ |
| `bool isExpanded(int index)` | _TODO_ |
| `int depth(int row)` | _TODO_ |
| `void expandRecursively(int row, int depth)` | _TODO_ |
| `void expandRecursively(int row)` | _TODO_ |
| `void expandRecursively()` | _TODO_ |
| `void collapseRecursively(int row)` | _TODO_ |
| `void collapseRecursively()` | _TODO_ |
| `void expandToIndex(QModelIndex index)` | _TODO_ |
| `void expandAll()` | _TODO_ |
| `void collapseAll()` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui

Terminal {
    TreeView {
        anchors.centerIn: parent
    }
}
```
