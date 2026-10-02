import QtQml 2.15
import Terminal 1.0

Rectangle {
    id: control
    property string label: ""
    property bool checked: false
    property var buttonGroup: null
    property string textColor: "white"
    property string radioColor: activeFocus ? "yellow" : "cyan"
    property string focusColor: "yellow"
    
    signal toggled(bool checked)

    function toggle() {
        if (!control.checked) {
            control.checked = true
            control.toggled(control.checked)
        }
    }

    function _uncheckGroup() {
        if (control.buttonGroup && control.buttonGroup.buttons) {
            var groupButtons = control.buttonGroup.buttons
            for (var i = 0; i < groupButtons.length; ++i) {
                var b = groupButtons[i]
                if (b !== control && b.checked !== undefined) {
                    b.checked = false
                }
            }
        }
    }

    onCheckedChanged: {
        if (checked) {
            _uncheckGroup()
        }
    }

    focusable: true
    width: 4 + (label !== "" ? label.length + 1 : 0)
    height: 1
    color: "transparent"

    Row {
        spacing: 1
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.checked ? "(*)" : "( )"
            color: control.radioColor
            bold: control.activeFocus
        }

        Text {
            visible: control.label !== ""
            text: control.label
            color: control.activeFocus ? control.focusColor : control.textColor
            bold: control.activeFocus
        }
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return) {
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
