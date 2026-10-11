import QtQml
import Qamui

Rectangle {
    id: status
    height: 3
    color: theme.surface

    required property var theme
    property string statusMessage: ""
    property string activeSearchInfo: ""

    border.style: "single"
    border.color: theme.border

    Row {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: 2

        Text {
            text: status.statusMessage.length > 0 ? status.statusMessage :
                  (status.activeSearchInfo.length > 0 ? status.activeSearchInfo : "Ready")
            color: status.statusMessage.length > 0 ? theme.success :
                   (status.activeSearchInfo.length > 0 ? theme.warning : theme.textPrimary)
            bold: true
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            text: "[Tab] Section  [j/k] Scroll  [s] Style  [e] Editor  [l] Lines  [/] Find  [t] Theme  [o] Sample  [q] Quit"
            color: theme.textMuted
        }
    }
}
