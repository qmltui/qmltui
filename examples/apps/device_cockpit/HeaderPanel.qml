import QtQml
import Qtui
import Qtui.Controls
import Qtui.Examples.DeviceCockpit

Rectangle {
    id: header
    height: 3
    color: Theme.bgHeader
    border.style: "double"
    border.color: Theme.borderNormal

    property string clockText: Qt.formatTime(new Date(), "hh:mm:ss")

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: header.clockText = Qt.formatTime(new Date(), "hh:mm:ss")
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Text {
            text: "◈ " + CockpitBackend.productName
            color: Theme.textAccent
            bold: true
        }
        Text {
            text: CockpitBackend.systemInfo.hostname
            color: Theme.textPrimary
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        StatusBadge {
            status: CockpitBackend.mockMode ? "warning" : "success"
            text: CockpitBackend.mockMode ? "MOCK" : "LIVE"
            showIcon: false
        }
        Text {
            text: header.clockText
            color: Theme.textSecondary
        }
    }
}
