import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    width: 44
    height: 9
    anchors.centerIn: parent
    visible: false

    property string title: "CONFIRM ACTION"
    property string message: "Are you sure you want to proceed?"
    signal confirmed()
    signal cancelled()

    color: "#1c1c1c"
    border.style: "double"
    border.color: HmiTheme.textWarning

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

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

        Text { text: ""; height: 1 } // Spacer

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "[Y] CONFIRM       [N] CANCEL"
            color: HmiTheme.textWarning
            bold: true
        }
    }
}
