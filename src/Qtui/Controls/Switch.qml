import QtQml
import Qtui

// A checkable control drawn as `[ ON]` or `[OFF]` followed by its label;
// both states are five cells wide so the label never shifts. Behaves like
// CheckBox: Space, Enter or a click toggles it.
AbstractButton {
    id: control
    checkable: true

    contentItem: Row {
        spacing: 1
        Text {
            text: control.checked ? "[ ON]" : "[OFF]"
            color: control.textColor
            bold: control.activeFocus || control.hovered || control.highlighted
        }
        Text {
            visible: control.text !== ""
            text: control.text
            color: control.textColor
            bold: control.activeFocus || control.hovered || control.highlighted
        }
    }
}
