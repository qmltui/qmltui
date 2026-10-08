---
title: CheckBox
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: A focusable two-state option drawn as [x] or [ ] followed by a label
---

# CheckBox QML type

A focusable two-state option drawn as `[x]` or `[ ]` followed by a label. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the label drawn one cell to the right of the indicator; an empty label hides that cell entirely. |
| `checked` | `bool` | `false` | Holds whether the box is checked and therefore drawn as `[x]` instead of `[ ]`. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour of the indicator and label, where `"default"` is the terminal's own foreground colour. |

## Signals

| Signal | Description |
| :--- | :--- |
| `toggled(bool checked)` | Emitted with the new state after the user flips the box with Space, Enter or a mouse click; it is not emitted when `checked` is set from code. |

## Detailed Description

CheckBox is a [Rectangle](rectangle.md) containing a `Row` of two
[Text](text.md) items: the indicator, `[x]` when `checked` and `[ ]`
otherwise, and the label, separated by one cell of spacing. The label is
hidden when `text` is empty, so a bare indicator is 3 cells wide. The
implicit size is the row's implicit size, one row high, and the row is
vertically centred if the item is made taller.

While the check box has active focus, Space and Enter (`Qt.Key_Return`) flip
`checked`, emit `toggled()` with the new value and accept the key; the
keyboard path does nothing when `enabled` is false. A mouse click anywhere on
the item gives it focus and toggles it the same way. Focus is shown by drawing
both indicator and label bold; the default `Rectangle` border is not used, so
the control occupies exactly one row. Tab and Shift+Tab move focus between
focusable items, and CheckBox is focusable whenever it is `enabled`.

Setting `checked` from a binding or in a handler changes the drawing but does
not emit `toggled()`; react to `onCheckedChanged` if you need to observe
programmatic changes as well. For a mutually exclusive choice use
[RadioButton](radiobutton.md) with a [ButtonGroup](buttongroup.md); for an
on/off look use [Switch](switch.md), which behaves identically but draws
`[ ON]` and `[OFF]`.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        CheckBox {
            id: logging
            text: "Enable logging"
            checked: true
            focus: true
            onToggled: checked => status.text = "Logging: " + (checked ? "on" : "off")
        }

        Text {
            id: status
            text: "Logging: on"
        }
    }
}
```
