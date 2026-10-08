import QtQml
import Qtui

Rectangle {
    id: control
    property string text: ""
    property bool checked: false
    property ButtonGroup buttonGroup: null
    property rgba textColor: "default"
    
    signal toggled(bool checked)

    function toggle() {
        if (control.enabled && !control.checked) {
            control.checked = true
            control.toggled(control.checked)
        }
    }

    QtObject {
        id: d
        // Exclusivity lives here rather than in ButtonGroup so it also applies
        // to programmatic and initial checked: true.
        function uncheckGroup() {
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
    }

    onCheckedChanged: {
        if (control.checked) {
            d.uncheckGroup()
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
            color: control.textColor
            bold: control.activeFocus
        }

        Text {
            visible: control.text !== ""
            text: control.text
            color: control.textColor
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
            if (!control.enabled)
                return
            control.focus = true
            control.toggle()
        }
    }
}
