import QtQml
import QmlTui

// A checkable control drawn as `[x]` or `[ ]` followed by its label, one row
// high. The indicator is drawn in reverse video while the control has active
// focus, so focus stays visible on terminals whose bold face barely differs;
// indicator and label are bold while focused, hovered or highlighted. Space or
// Enter while focused, or a click, toggles it and emits toggled(checked) then
// clicked(). Programmatic changes to `checked` emit neither.
AbstractButton {
    id: control
    checkable: true

    contentItem: Row {
        spacing: 1
        Text {
            text: control.checked ? "[x]" : "[ ]"
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
