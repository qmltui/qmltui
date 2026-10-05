import QtQml
import Qtui

Column {
    id: agentCard
    required property string content
    required property string title
    required property string extra

    readonly property var lines: agentCard.content.split("\n")

    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "✦ " + (agentCard.title || "Agent")
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " (" + (agentCard.extra || "2.1k tokens · 0.8s") + ")"
            color: "#64748b"
        }
    }

    Repeater {
        model: agentCard.lines
        delegate: Text {
            required property string modelData
            text: "  " + modelData
            color: "white"
        }
    }
}
