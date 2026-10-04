import QtQuick
import Qtui
import Qtui.Controls
import Qtui.Extras

Rectangle {
    id: toast

    Theme { id: theme }

    property string message: ""
    property string status: "success"
    property int displayDuration: 3500

    width: Math.min(50, message.length + 8)
    height: 1
    color: "#1e293b"
    border.style: "single"
    border.color: status === "success" ? theme.green : (status === "warning" ? theme.amber : theme.cyan)
    visible: false

    function notify(msg, stat) {
        message = msg || ""
        if (stat) status = stat
        visible = true
        dismissTimer.restart()
    }

    Timer {
        id: dismissTimer
        interval: toast.displayDuration
        repeat: false
        onTriggered: toast.visible = false
    }

    Row {
        anchors.fill: parent
        spacing: 1

        StatusBadge {
            text: toast.status.toUpperCase()
            status: toast.status
            variant: "subtle"
            showIcon: false
        }

        Text {
            text: toast.message
            color: "white"
            bold: true
        }
    }
}
