---
title: CheckBox
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: AbstractButton
cpp: qmltui::Item
category: inputs
summary: A focusable two-state option drawn as [x] or [ ] followed by a label
---

# CheckBox QML type

A focusable two-state option drawn as `[x]` or `[ ]` followed by a label. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [AbstractButton](abstractbutton.md) |

CheckBox adds no members of its own: `checked`, `text`, `textColor`,
`toggled()` and the rest are inherited from
[AbstractButton](abstractbutton.md), with `checkable` set to true.

## Detailed Description

CheckBox is a checkable [AbstractButton](abstractbutton.md) whose
`contentItem` is a `Row` of two [Text](text.md) items: the indicator, `[x]`
when `checked` and `[ ]` otherwise, and the label one cell to its right. The
label is hidden when `text` is empty, so a bare indicator is 3 cells wide. It
has no `background`, no paddings and no border, so it occupies exactly one
row at its implicit size and centres the row when made taller. Indicator and
label are bold while the control has active focus, is `hovered`, or is `highlighted`;
the indicator is additionally drawn in reverse video while the control has active
focus, so the focused control stays visible on terminals whose bold face barely
differs from the regular one.

Space or Enter while focused, or a mouse click anywhere on the item, calls
`toggle()`, which flips `checked` and emits `toggled()` with the new value,
then emits `clicked()`. Setting `checked` from a binding or a handler changes
the drawing but emits neither signal; observe `onCheckedChanged` for that.
Nothing happens while `enabled` is false. For a mutually exclusive choice use
[RadioButton](radiobutton.md) with a [ButtonGroup](buttongroup.md); for an
on/off look use [Switch](switch.md), which behaves identically but draws
`[ ON]` and `[OFF]`. Assign `contentItem` to change the indicator glyphs.

### Example

```qml
import QtQml
import QmlTui
import QmlTui.Controls

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

### Terminal Preview

```text
[x] Enable logging

Logging: on
```
