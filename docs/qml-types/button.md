---
title: Button
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: AbstractButton
cpp: qmltui::Item
category: inputs
summary: A focusable, bordered action button that emits clicked() on Enter, Space or a mouse click
---

# Button QML type

A focusable, bordered action button that emits `clicked()` on Enter, Space or a mouse click. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [AbstractButton](abstractbutton.md) |

Button adds no members of its own; every property, signal and method is
inherited from [AbstractButton](abstractbutton.md).

## Detailed Description

Button is an [AbstractButton](abstractbutton.md) with a bordered
[Rectangle](rectangle.md) as `background` and a centred [Text](text.md) label
as `contentItem`, with paddings of 2 cells horizontally and 1 row vertically
so the implicit size is the label width plus 4 by 3 rows. Enter or Space while
focused, or a mouse press and release inside it, emits `clicked()`; a press
also gives the button focus and holds `pressed`. Nothing happens while
`enabled` is false. Tab and Shift+Tab move focus between focusable items.

State shows in the border and the label, and each state has its own look.
The border style is `"single"`, or `"double"` while the button has active
focus, and it is dropped on a button shorter than 3 rows, where the label is
centred on the single row and shows focus by boldness alone. While `down`,
that is while a mouse button is held on it, the label is drawn in reverse
video, so a click flashes and the double border that remains afterwards is
focus, which a click also gives the button. The label is bold while the control has active focus, is `hovered`, or is `highlighted`.
Hover is decoration only: keyboard focus reaches every state, and terminals or
recordings without motion reporting never show it. `textColor`, the
background colour and the border colour default to `"default"`, the terminal's
own colours.

To restyle a button, assign `background`, `contentItem`, or both; the button's
properties are reachable through its `id`. Setting `color` or `border` on the
Button itself is no longer possible, since those belonged to the Rectangle it
used to be:

```qml
Button {
    id: filled
    text: "Filled"
    background: Rectangle { color: "#65e6a0"; border.style: Border.None }
    contentItem: Text {
        text: filled.text
        color: "#0b0f14"
        bold: filled.activeFocus
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
```

For a toggle use [CheckBox](checkbox.md) or [Switch](switch.md), for one
choice out of several [RadioButton](radiobutton.md) with a
[ButtonGroup](buttongroup.md).

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

        Button {
            text: "Run"
            focus: true
            onClicked: status.text = "Running..."
        }

        Text {
            id: status
            text: "Idle"
        }
    }
}
```
