---
title: RadioButton
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: A focusable exclusive option drawn as (*) or ( ) that unchecks its ButtonGroup siblings when selected
---

# RadioButton QML type

A focusable exclusive option drawn as `(*)` or `( )` that unchecks its [ButtonGroup](buttongroup.md) siblings when selected. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the label drawn one cell to the right of the indicator; an empty label hides that cell entirely. |
| `checked` | `bool` | `false` | Holds whether this option is selected and therefore drawn as `(*)` instead of `( )`; becoming checked unchecks every other member of `buttonGroup`. |
| `buttonGroup` | `ButtonGroup` | `null` | Holds the group whose other members are unchecked when this button becomes checked; it is assigned automatically when the button is listed in a `ButtonGroup.buttons`. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour of the indicator and label, where `"default"` is the terminal's own foreground colour. |

## Signals

| Signal | Description |
| :--- | :--- |
| `toggled(bool checked)` | Emitted with `true` when the user selects this button with Space, Enter, a mouse click or `toggle()`; it is not emitted when the button is unchecked by a sibling or when `checked` is set from code. |

## Methods

| Method | Description |
| :--- | :--- |
| `var toggle()` | Checks the button and emits `toggled(true)` if it is enabled and not already checked; it never unchecks. |

## Detailed Description

RadioButton is a [Rectangle](rectangle.md) containing a `Row` of two
[Text](text.md) items: the indicator, `(*)` when `checked` and `( )`
otherwise, and the label, separated by one cell of spacing. The label is
hidden when `text` is empty. The implicit size is the row's implicit size,
one row high, and the row is vertically centred if the item is made taller.
Focus is shown by drawing both indicator and label bold; no border is drawn.
Tab and Shift+Tab move focus between focusable items, and RadioButton is
focusable whenever it is `enabled`, and a disabled radio button ignores both
keys and clicks.

While the button has active focus, Space and Enter (`Qt.Key_Return`) call
`toggle()` and accept the key; a mouse click anywhere on the item gives it
focus and calls `toggle()` as well. `toggle()` only ever checks: a selected
radio button cannot be deselected by the user, only by selecting another
member of the same group. Exclusivity is implemented in RadioButton itself
through `onCheckedChanged`: whenever `checked` becomes true, including when it
is set from code or as an initial value, every other item in
`buttonGroup.buttons` that has a `checked` property is set to false. The
unchecked siblings do not emit `toggled()`, so connect to `onToggled` on each
button to learn which one was selected, or bind to `checked` directly.

Without a [ButtonGroup](buttongroup.md), radio buttons are independent and
several can be checked at once. Assign the buttons to a group's `buttons`
list and the group sets `buttonGroup` on each of them. If more than one member
starts with `checked: true`, the one initialised last wins.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

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
