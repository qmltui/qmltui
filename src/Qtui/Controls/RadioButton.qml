import QtQml
import Qtui

// One choice out of a group, drawn as `(*)` or `( )` followed by its label.
// autoExclusive: activating a checked radio button keeps it checked. Put
// the buttons of one choice in a ButtonGroup; checking one, by the user or
// from code, unchecks the others.
AbstractButton {
    id: control
    checkable: true
    autoExclusive: true
    property ButtonGroup buttonGroup: null

    QtObject {
        id: d
        // Exclusivity lives here rather than in ButtonGroup so it also applies
        // to programmatic and initial checked: true.
        function uncheckGroup() {
            if (control.buttonGroup && control.buttonGroup.buttons) {
                const groupButtons = control.buttonGroup.buttons
                for (let i = 0; i < groupButtons.length; ++i) {
                    const b = groupButtons[i]
                    if (b !== control && b.checked !== undefined)
                        b.checked = false
                }
            }
        }
    }

    onCheckedChanged: {
        if (control.checked)
            d.uncheckGroup()
    }

    contentItem: Row {
        spacing: 1
        Text {
            text: control.checked ? "(*)" : "( )"
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
