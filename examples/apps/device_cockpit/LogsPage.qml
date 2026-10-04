import QtQml
import Qtui
import Qtui.Controls
import Qtui.Examples.DeviceCockpit

Rectangle {
    id: page
    color: "transparent"

    readonly property var logs: CockpitBackend.logs

    onVisibleChanged: {
        if (visible)
            logView.focus = true
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_F) {
            logView.followTail = !logView.followTail
            event.accepted = true
        } else if (event.key === Qt.Key_C) {
            logView.clear()
            event.accepted = true
        }
    }

    Component.onCompleted: {
        const lines = page.logs.backlog()
        for (let i = 0; i < lines.length; ++i)
            logView.appendLine(lines[i])
    }

    Connections {
        target: page.logs
        function onLineAppended(line) {
            logView.appendLine(line)
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: parent.width
            height: 3
            color: Theme.bgPanel
            border.style: "single"
            border.color: Theme.borderNormal

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text { text: "LIVE JOURNAL"; color: Theme.textAccent; bold: true }
                StatusBadge {
                    status: page.logs.available ? "success" : "error"
                    text: page.logs.available ? "STREAMING" : "UNAVAILABLE"
                    showIcon: false
                }
                Text {
                    text: "FOLLOW: " + (logView.followTail ? "ON" : "OFF")
                    color: logView.followTail ? Theme.textAccent : Theme.textWarning
                    bold: true
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                text: "[F] Follow  [C] Clear  [↑↓/PgUp/PgDn] Scroll"
                color: Theme.textDim
            }
        }

        LogView {
            id: logView
            width: parent.width
            height: parent.height - 3
            readOnly: true
            followTail: true
            maximumLineCount: 1000
            color: Theme.textSecondary
        }
    }
}
