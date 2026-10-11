import QtQml
import Qamui
import IndustrialPlcHmi

Rectangle {
    id: root
    property string title: "CONFIRM ACTION"
    property string message: "Are you sure you want to proceed?"
    signal confirmed()
    signal cancelled()

    width: Math.min(64, parent ? Math.max(36, parent.width - 4) : 64)
    height: 7
    anchors.centerIn: parent
    visible: false
    focus: visible
    z: 100
    color: "#1c1c1c"
    border.style: "double"
    border.color: HmiTheme.textWarning

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Rectangle {
            width: parent.width
            height: 1
            color: HmiTheme.textWarning

            Text {
                anchors.centerIn: parent
                text: "⚠️  " + root.title
                color: "black"
                bold: true
            }
        }

        Text {
            width: parent.width
            text: root.message
            color: HmiTheme.textPrimary
        }

        Text { text: ""; height: 1 }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "[Y] CONFIRM       [N] CANCEL"
            color: HmiTheme.textWarning
            bold: true
        }
    }

    function open(dialogTitle, dialogMessage) {
        root.title = dialogTitle
        root.message = dialogMessage
        root.visible = true
    }

    function accept() {
        root.visible = false
        root.confirmed()
    }

    function reject() {
        root.visible = false
        root.cancelled()
    }
}
