import QtQml
import Qtui

Rectangle {
    id: control
    property string text: ""
    property bool checked: false
    property ButtonGroup buttonGroup: null
    property rgba textColor: Palette.windowText
    
    signal toggled(bool checked)

    function toggle() {
        if (control.enabled && !control.checked) {
            control.checked = true
            control.toggled(control.checked)
        }
    }

    function _uncheckGroup() {
        if (control.buttonGroup && control.buttonGroup.buttons) {
            const groupButtons = control.buttonGroup.buttons
            for (let i = 0; i < groupButtons.length; ++i) {
                const b = groupButtons[i]
                if (b !== control && b.checked !== undefined) {
                    b.checked = false
                }
            }
        }
    }

    onCheckedChanged: {
        if (control.checked) {
            control._uncheckGroup()
        }
    }

    focusable: enabled
    implicitWidth: contentRow.implicitWidth
    implicitHeight: contentRow.implicitHeight
    Row {
        id: contentRow
        spacing: 1
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.checked ? "(*)" : "( )"
            color: !control.enabled ? Palette.placeholderText : (control.activeFocus ? Palette.highlight : Palette.accent)
            bold: control.activeFocus
        }

        Text {
            visible: control.text !== ""
            text: control.text
            color: !control.enabled ? Palette.placeholderText : (control.activeFocus ? Palette.highlight : control.textColor)
            bold: control.activeFocus
        }
    }

    Keys.onPressed: (event) => {
        if (control.enabled && (event.key === Qt.Key_Space || event.key === Qt.Key_Return)) {
            control.toggle()
            event.accepted = true
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (event) => {
            control.focus = true
            control.toggle()
        }
    }
}
