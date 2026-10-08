import QtQml
import Qtui

// A checkable control drawn as `[x]` or `[ ]` followed by its label, one row
// high, bold while focused. Space or Enter while focused, or a click, toggles
// it and emits toggled(checked) then clicked(). Programmatic changes to
// `checked` emit neither.
AbstractButton {
    id: control
    checkable: true

    contentItem: Row {
        spacing: 1
        Text {
            text: control.checked ? "[x]" : "[ ]"
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
