---
title: StandardItemDelegate
kind: qml
import: Qamui.Controls
since: "0.4"
inherits: Item
cpp: qamui::Item
category: data views
summary: A one-row view delegate that renders Qt's standard item roles the way QStyledItemDelegate does in Qt Widgets.
---

# StandardItemDelegate QML type

A one-row view delegate that renders Qt's standard item roles the way QStyledItemDelegate does in Qt Widgets. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui.Controls` |
| **Since** | Qamui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | Required. Holds the row supplied by the view: a map of role names for a `QAbstractItemModel` row, or the raw value for a JavaScript array entry. |
| `current` | `bool` |  | Whether the row is drawn as the current one, bold and in reverse video; bound to `ListView.isCurrentItem` and set by the other views. |
| `textColor` | `rgba` | `"default"` | Holds the text colour used when the row has no `foreground` role, where `"default"` is the terminal's own foreground. |
| `display` | `string` |  | Read-only. Holds the row text: the value itself for a string or number, else `model.display`, else `model.text`, else empty. |
| `checkable` | `bool` |  | Read-only. True when the model flags the row `Qt::ItemIsUserCheckable`, published by the view as `model.checkable`. |
| `checkState` | `int` |  | Read-only. Holds `model.checkState` from `Qt::CheckStateRole` (`Qt.Unchecked`, `Qt.PartiallyChecked` or `Qt.Checked`), or `-1` when the row has no check state. |
| `itemEnabled` | `bool` |  | Read-only. False only when the view publishes `model.enabled` as false, meaning the row lacks `Qt::ItemIsEnabled`. |
| `foreground` | `string` |  | Read-only. Holds the colour string from `Qt::ForegroundRole`, or empty when unset. |
| `background` | `string` |  | Read-only. Holds the colour string from `Qt::BackgroundRole`, or empty when unset. |
| `decoration` | `string` |  | Read-only. Holds the glyph from a string or character `Qt::DecorationRole`, or empty when unset or an image. |
| `alignment` | `string` |  | Read-only. Holds `"left"`, `"center"` or `"right"` from `model.textAlignment` (`Qt::TextAlignmentRole`), defaulting to `"left"`. |
| `prefix` | `string` |  | Read-only. Holds the text drawn before `display`: the decoration glyph and a space, then `[x] `, `[-] ` or `[ ] ` when the row has a check state. |

## Signals

| Signal | Description |
| :--- | :--- |
| `toggled(int checkState)` | Emitted after `toggle()` has written the new check state, `Qt.Checked` or `Qt.Unchecked`, to the model. |

## Methods

| Method | Description |
| :--- | :--- |
| `var toggle()` | Flips the check state between `Qt.Checked` and `Qt.Unchecked` through the model's `setData()` on `Qt::CheckStateRole` and returns true; returns false without touching the model when the row is not checkable, is disabled, or has no valid `modelIndex`. |

## Detailed Description

StandardItemDelegate is the terminal counterpart of `QStyledItemDelegate`.
Qamui views publish Qt's numeric item roles, which have no entry in
`roleNames()`, to delegates under Widgets-style names: `foreground` and
`background` from `Qt::ForegroundRole` and `Qt::BackgroundRole` as colour
strings, `textAlignment` from `Qt::TextAlignmentRole`, `checkState` from
`Qt::CheckStateRole`, `decoration` from a string or character
`Qt::DecorationRole`, `enabled` and `checkable` from `flags()`, and
`modelIndex` for checkable rows. The delegate reads each of these from its
required `model` property with a fallback, so unstyled rows work with any
model, and takes its text from `display` (`Qt::DisplayRole`) or `text`. Font
roles are not interpreted; only `bold` and `reversed` are applied, and both
mark the current row.

Each instance is one row high, `width` bound to its parent's width and
`implicitWidth` to the label's. A `Rectangle` behind the text shows
`background` when set and the row is not current; the `Text` draws
`prefix + display` in `foreground` or `textColor`, aligned by `alignment`,
dimmed (SGR 2) and with `enabled` false when the row is disabled, and bold plus
reverse video when `current`. It works as the `delegate` of
[ListView](listview.md), [GridView](gridview.md), [Repeater](repeater.md),
[TableView](tableview.md) and [TreeView](treeview.md); `current` binds to
`ListView.isCurrentItem` and is assigned by the other views.

Space while the delegate itself has focus, or a mouse click anywhere on the
row, calls `toggle()`, which writes back through `setData()` and emits
`toggled()`; the view then repaints from `dataChanged`. A `ListView` that has
focus turns Space into its own `itemActivated` signal instead, so toggle from
that handler as `examples/components/standard_item_delegate/Main.qml` does.
See the [models guide](../topics/models.md) for the full role table.

### Example

```qml
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    Rectangle {
        anchors.centerIn: parent
        width: 30
        height: 7
        border.style: "single"

        ListView {
            anchors.fill: parent
            anchors.margins: 1
            focus: true
            currentIndex: 0
            model: ["Build", "Run tests", "Tag release", "Publish notes", "Announce"]
            delegate: StandardItemDelegate {
                textColor: "cyan"
            }
        }
    }
}
```

## Standard model roles


`Qamui.Controls.StandardItemDelegate` is the terminal counterpart of Qt
Widgets' `QStyledItemDelegate`: a delegate for any C++ model that renders the
standard item roles, decoration glyph, check box, foreground and background
colors, text alignment, and disabled state, plus reversed text for the current
row. It works as the `delegate` of `ListView`, `GridView`, `Repeater`,
`TableView`, and `TreeView`. `toggle()` flips `Qt::CheckStateRole` through
`setData()`; `toggled(checkState)` reports it. `current` binds to
`ListView.isCurrentItem` and is set by the other views. `textColor` styles
rows without a foreground role. The [models guide](../topics/models.md) lists the
role names every view provides; see the
[standard_item_delegate example](../../examples/components/standard_item_delegate/Main.qml).
