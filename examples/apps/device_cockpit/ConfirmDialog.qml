import QtQml
import Qtui
import Qtui.Controls
import Qtui.Examples.DeviceCockpit

Popup {
    id: control

    property string title: ""
    property string message: ""

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    implicitWidth: Math.max(44, contentColumn.implicitWidth + 4)
    implicitHeight: contentColumn.implicitHeight + 2
    border.style: "double"
    border.color: Theme.borderFocus
    color: Theme.bgHeader
    anchors.centerIn: parent
    initialFocusItem: confirmButton

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Y || event.key === Qt.Key_Return
                || event.key === Qt.Key_Enter) {
            control.accept()
            event.accepted = true
        } else if (event.key === Qt.Key_N) {
            control.reject()
            event.accepted = true
        }
    }

    Column {
        id: contentColumn
        x: 2
        y: 1
        spacing: 1

        Text {
            text: control.title
            bold: true
            color: Theme.textWarning
        }
        Text {
            text: control.message
            color: Theme.textPrimary
        }
        Row {
            spacing: 2

            Button {
                id: confirmButton
                text: "[Y] Confirm"
                focusColor: Theme.bgActiveTab
                onClicked: control.accept()
            }
            Button {
                text: "[N] Cancel"
                onClicked: control.reject()
            }
        }
    }
}
