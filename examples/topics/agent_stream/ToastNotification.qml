import QtQml
import QmlTui
import QmlTui.Controls

Rectangle {
    id: toast

    Theme { id: theme }

    property string message: ""
    property string status: "success"
    property int displayDuration: 3500

    width: Math.min(50, message.length + 8)
    height: 1
    color: toastMouse.containsMouse ? "#334155" : "#1e293b"
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
            text: toast.status
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

    MouseArea {
        id: toastMouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: toast.visible = false
    }
}
