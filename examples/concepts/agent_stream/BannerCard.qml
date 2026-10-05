import QtQuick
import Qtui
import Qtui.Controls

Column {
    id: banner
    required property string activeModel
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "✦ Qt AI studio"
            color: "#38bdf8"
            bold: true
        }
        StatusBadge {
            text: "v2.4"
            status: "info"
            variant: "subtle"
        }
        Text {
            text: " · " + banner.activeModel + " · ~/Qtui"
            color: "#94a3b8"
        }
    }
    Text {
        text: "  Type a prompt or /demo [1-4] to simulate agent workflows · /help for commands"
        color: "#64748b"
    }
}
