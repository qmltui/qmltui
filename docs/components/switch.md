---
title: Switch
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: A focusable on/off toggle drawn as [ ON] or [OFF] followed by a label
---

# Switch QML type

A focusable on/off toggle drawn as `[ ON]` or `[OFF]` followed by a label. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the label drawn one cell to the right of the indicator; an empty label hides that cell entirely. |
| `checked` | `bool` | `false` | Holds whether the switch is on and therefore drawn as `[ ON]` instead of `[OFF]`. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour of the indicator and label, where `"default"` is the terminal's own foreground colour. |

## Signals

| Signal | Description |
| :--- | :--- |
| `toggled(bool checked)` | Emitted with the new state after the user flips the switch with Space, Enter or a mouse click; it is not emitted when `checked` is set from code. |

## Detailed Description

Switch is a [Rectangle](rectangle.md) containing a `Row` of two
[Text](text.md) items: the indicator, `[ ON]` when `checked` and `[OFF]`
otherwise, and the label, separated by one cell of spacing. Both states are 5
cells wide so the label does not move when the switch flips. The label is
hidden when `text` is empty. The implicit size is the row's implicit size, one
row high, and the row is vertically centred if the item is made taller.

While the switch has active focus, Space and Enter (`Qt.Key_Return`) flip
`checked`, emit `toggled()` with the new value and accept the key; the
keyboard path does nothing when `enabled` is false. A mouse click anywhere on
the item gives it focus and toggles it the same way. Focus is shown by
drawing both indicator and label bold; no border is drawn. Tab and Shift+Tab
move focus between focusable items, and Switch is focusable whenever it is
`enabled`.

Switch is behaviourally identical to [CheckBox](checkbox.md) and differs only
in the indicator text; pick whichever reads better for the setting. Setting
`checked` from code changes the drawing but does not emit `toggled()`.

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

        Switch {
            text: "Dark theme"
            checked: true
            focus: true
            onToggled: checked => status.text = "Theme: " + (checked ? "dark" : "light")
        }

        Text {
            id: status
            text: "Theme: dark"
        }
    }
}
```
