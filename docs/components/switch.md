---
title: Switch
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: AbstractButton
cpp: qtui::Item
category: inputs
summary: A focusable on/off toggle drawn as [ ON] or [OFF] followed by a label
---

# Switch QML type

A focusable on/off toggle drawn as `[ ON]` or `[OFF]` followed by a label. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [AbstractButton](abstractbutton.md) |

Switch adds no members of its own: `checked`, `text`, `textColor`,
`toggled()` and the rest are inherited from
[AbstractButton](abstractbutton.md), with `checkable` set to true.

## Detailed Description

Switch is a checkable [AbstractButton](abstractbutton.md) whose `contentItem`
is a `Row` of two [Text](text.md) items: the indicator, `[ ON]` when
`checked` and `[OFF]` otherwise, both five cells wide so the label never
shifts, and the label one cell to its right, hidden when `text` is empty. It
has no `background` and no paddings, so it occupies one row at its implicit
size. Indicator and label are bold while the control has active focus, is `hovered`, or is `highlighted`.

It behaves exactly like [CheckBox](checkbox.md): Space, Enter or a click calls
`toggle()`, emitting `toggled()` with the new value and then `clicked()`;
assigning `checked` from code emits neither. Choose Switch where the option
reads as on or off rather than as a ticked item.

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
