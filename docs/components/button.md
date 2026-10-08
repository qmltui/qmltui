---
title: Button
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Rectangle
cpp: qtui::Rectangle
category: inputs
summary: A focusable, bordered action button that emits clicked() on Enter, Space or a mouse click
---

# Button QML type

A focusable, bordered action button that emits `clicked()` on Enter, Space or a mouse click. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Rectangle](rectangle.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the label drawn centred inside the button. |
| `pressed` | `bool` | `false` | Holds whether a mouse button is currently held down on the button; keyboard activation does not set it. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour of the label, where `"default"` is the terminal's own foreground colour. |

## Signals

| Signal | Description |
| :--- | :--- |
| `clicked()` | Emitted when the button is activated with Enter or Space while focused, or when a mouse button is pressed and released inside it. |

## Detailed Description

Button is a [Rectangle](rectangle.md) with a centred [Text](text.md) label
and a `MouseArea` filling it. While the button has active focus, Enter
(`Qt.Key_Return`) and Space emit `clicked()` and accept the key event; the
keyboard path does nothing when `enabled` is false, and neither does the
mouse path; the engine also withholds keys and clicks from disabled items. Pressing a mouse button on
the item gives it focus and sets `pressed`, releasing clears `pressed`, and a
press-and-release inside the item emits `clicked()`. Tab and Shift+Tab move
focus between focusable items, and Button is focusable whenever it is
`enabled`.

Focus is shown by the border and the label: the border style is `"single"`
normally and `"double"` while the button has active focus, and the label is
drawn bold while focused. The border is only drawn when `height` is at least 3
cells; a shorter button has `border.style` `"none"`, so a one-line button
shows focus through the bold label alone. `color`, `border.color` and
`textColor` all default to `"default"`, the terminal's own colours, and can be
overridden per instance or in a derived component.

The implicit size is the label's implicit width plus 4 cells (one border and
one padding cell on each side) by 3 rows. Unlike [CheckBox](checkbox.md) and
[Switch](switch.md), Button holds no state of its own beyond `pressed`; use
`clicked()` to drive application state.

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
