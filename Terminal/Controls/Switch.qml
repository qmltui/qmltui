import QtQml
import Qtui

Rectangle {
    id: control
    property string label: ""
    property bool checked: false
    property string textColor: "white"
    property string onColor: "green"
    property string offColor: "red"
    property string focusColor: "yellow"
    
    signal toggled(bool checked)

    focusable: enabled
    implicitWidth: contentRow.implicitWidth
    implicitHeight: contentRow.implicitHeight
    Row {
        id: contentRow
        spacing: 1
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.checked ? "[ ON]" : "[OFF]"
            color: !control.enabled ? "gray" : (control.activeFocus ? control.focusColor : (control.checked ? control.onColor : control.offColor))
            bold: control.activeFocus
        }

        Text {
            visible: control.label !== ""
            text: control.label
            color: !control.enabled ? "gray" : (control.activeFocus ? control.focusColor : control.textColor)
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
