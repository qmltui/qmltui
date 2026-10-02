import QtQml 2.15
import Terminal 1.0

Rectangle {
    id: control
    property string label: ""
    property bool checked: false
    property string textColor: "white"
    property string checkColor: activeFocus ? "yellow" : "cyan"
    property string focusColor: "yellow"
    
    signal toggled(bool checked)

    focusable: true
    width: 4 + (label !== "" ? label.length + 1 : 0)
    height: 1
    color: "transparent"

    Row {
        spacing: 1
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.checked ? "[x]" : "[ ]"
            color: control.checkColor
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
            control.checked = !control.checked
            control.toggled(control.checked)
            event.accepted = true
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (event) => {
            control.focus = true
            control.checked = !control.checked
            control.toggled(control.checked)
        }
    }
}
