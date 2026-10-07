import QtQml
import Qtui

Rectangle {
    id: control
    property string text: ""
    property bool checked: false
    property rgba textColor: "default"

    signal toggled(bool checked)

    focusable: enabled
    implicitWidth: contentRow.implicitWidth
    implicitHeight: contentRow.implicitHeight
    Row {
        id: contentRow
        spacing: 1
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.checked ? "[x]" : "[ ]"
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
