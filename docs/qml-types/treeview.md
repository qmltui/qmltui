---
title: TreeView
kind: qml
import: Qamui
since: "0.4"
inherits: Item
cpp: qamui::TreeView
category: data views
summary: Displays expandable hierarchical model data.
---

# TreeView QML type

Displays expandable hierarchical model data. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
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
import Qamui

Terminal {
    TreeView {
        anchors.centerIn: parent
    }
}
```

## Usage guide

For `QAbstractItemModel`, set `model`; optionally set `rootIndex` to display a
subtree. Terminal-native map trees use `rootNode` and a `children` list. Keeping
these entry points separate avoids overloading Qt Quick's `model` semantics.

Properties include `model`, `rootNode`, `rootIndex`, `delegate`, `currentIndex`,
`currentRow`, `currentColumn`, and `currentItemData`. Changes to the current
node's roles notify `currentItemDataChanged`.

Methods include `expand`, `collapse`, `toggleExpanded`, `isExpanded`, `depth`,
`expandRecursively`, `collapseRecursively`, `expandToIndex`, `expandAll`, and
`collapseAll`. Expansion emits `expanded(row, depth)` or
`collapsed(row, recursively)`.

For C++ models, expansion and the current node are tracked by persistent
index, so renames, inserts above, proxy sorting, and moves preserve them.
`dataChanged` refreshes visible nodes in place without re-creating delegates.
Expanding a branch that reports `canFetchMore()` calls `fetchMore()` on it.
A reset collapses everything, and a proxy that filters a branch out drops its
expansion state, matching Qt's persistent-index semantics.

See the [TreeView example](../../examples/components/tree_view/Main.qml).
