---
title: Item
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Item
category: core
summary: _TODO_
---

# Item QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `x` | `int` |  | _TODO_ |
| `y` | `int` |  | _TODO_ |
| `z` | `real` |  | _TODO_ |
| `width` | `int` |  | _TODO_ |
| `height` | `int` |  | _TODO_ |
| `implicitWidth` | `int` |  | _TODO_ |
| `implicitHeight` | `int` |  | _TODO_ |
| `visible` | `bool` |  | _TODO_ |
| `enabled` | `bool` |  | _TODO_ |
| `focus` | `bool` |  | _TODO_ |
| `activeFocus` | `bool` |  | Read-only. _TODO_ |
| `focusable` | `bool` |  | _TODO_ |
| `parent` | `Item` |  | _TODO_ |
| `anchors` | `Anchors` |  | Read-only. _TODO_ |
| `left` | `AnchorLine` |  | Read-only. _TODO_ |
| `right` | `AnchorLine` |  | Read-only. _TODO_ |
| `top` | `AnchorLine` |  | Read-only. _TODO_ |
| `bottom` | `AnchorLine` |  | Read-only. _TODO_ |
| `horizontalCenter` | `AnchorLine` |  | Read-only. _TODO_ |
| `verticalCenter` | `AnchorLine` |  | Read-only. _TODO_ |
| `clip` | `bool` |  | _TODO_ |
| `padding` | `int` |  | _TODO_ |
| `leftPadding` | `int` |  | _TODO_ |
| `rightPadding` | `int` |  | _TODO_ |
| `topPadding` | `int` |  | _TODO_ |
| `bottomPadding` | `int` |  | _TODO_ |
| `data` | `list<QtObject>` |  | Read-only. _TODO_ |
| `children` | `list<Item>` |  | Read-only. _TODO_ |
| `states` | `list<State>` |  | Read-only. _TODO_ |
| `transitions` | `list<Transition>` |  | Read-only. _TODO_ |
| `state` | `string` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `dirty()` | _TODO_ |
| `ancestryChanged()` | The item or an ancestor was attached to or detached from a parent, so the item's root [Terminal](terminal.md) may have changed. Unlike `parentChanged`, an ancestor's reparenting — a `Repeater` or `Loader` inserting finished content — reaches every item of the moved subtree. Emitted on each item; connect on the item whose Terminal-derived state needs refreshing. |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qamui

Terminal {
    Item {
        anchors.centerIn: parent
    }
}
```

## 13. QML Exposure

Registered as `Item` in the `Tui` QML module via `QML_ELEMENT`.

