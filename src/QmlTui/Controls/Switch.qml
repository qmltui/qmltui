import QtQml
import QmlTui

// A checkable control drawn as `[ ON]` or `[OFF]` followed by its label;
// both states are five cells wide so the label never shifts. The indicator
// is drawn in reverse video while the control has active focus; indicator
// and label are bold while focused, hovered or highlighted. Behaves like
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
            reversed: control.activeFocus
        }
        Text {
            visible: control.text !== ""
            text: control.text
            color: control.textColor
            bold: control.activeFocus || control.hovered || control.highlighted
        }
    }
}
