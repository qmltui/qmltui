---
title: RadioButton
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: AbstractButton
cpp: qmltui::Item
category: inputs
summary: A focusable exclusive option drawn as (*) or ( ) that unchecks its ButtonGroup siblings when selected
---

# RadioButton QML type

A focusable exclusive option drawn as `(*)` or `( )` that unchecks its [ButtonGroup](buttongroup.md) siblings when selected. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [AbstractButton](abstractbutton.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `buttonGroup` | `ButtonGroup` | `null` | Holds the group this button belongs to; [ButtonGroup](buttongroup.md) assigns it to each member. Checking this button unchecks every other member. |

`checked`, `text`, `textColor`, `toggle()`, `toggled()` and the rest are
inherited from [AbstractButton](abstractbutton.md), with `checkable` and
`autoExclusive` set to true.

## Detailed Description

RadioButton is a checkable, `autoExclusive`
[AbstractButton](abstractbutton.md) whose `contentItem` is a `Row` of two
[Text](text.md) items: the indicator, `(*)` when `checked` and `( )`
otherwise, and the label one cell to its right, hidden when `text` is empty.
It has no `background` and no paddings, so it occupies one row at its
implicit size. Indicator and label are bold while the control has active focus,
is `hovered`, or is `highlighted`; the indicator is additionally drawn in reverse
video while the control has active focus.

Space or Enter while focused, or a click, calls `toggle()`. Because the
button is `autoExclusive`, `toggle()` checks an unchecked button and emits
`toggled(true)` then `clicked()`, but leaves a checked one checked, so a
group never has nothing selected. Exclusivity is enforced from
`onCheckedChanged`: whenever this button becomes checked, every other member
of its `buttonGroup` is unchecked. That applies to user activation, to
`checked` assigned from code, and to an initial `checked: true`, where the
last button initialised wins. Unchecked siblings do not emit `toggled()`.
Without a [ButtonGroup](buttongroup.md), radio buttons are independent
toggles that can only be checked.

### Example

```qml
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    ButtonGroup {
        buttons: [small, large]
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        RadioButton {
            id: small
            text: "Small"
            checked: true
            focus: true
            onToggled: status.text = "Size: small"
        }

        RadioButton {
            id: large
            text: "Large"
            onToggled: status.text = "Size: large"
        }

        Text {
            id: status
            text: "Size: small"
        }
    }
}
```

### Terminal Preview

```text
(*) Small

( ) Large

Size: small
```
