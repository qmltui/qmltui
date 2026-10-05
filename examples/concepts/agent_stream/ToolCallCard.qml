import QtQml
import Qtui

Column {
    id: toolCallCard
    required property string title
    required property string content
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "⚡"
            color: "#fbbf24"
            bold: true
        }
        Text {
            text: " " + toolCallCard.title
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " " + toolCallCard.content
            color: "#94a3b8"
        }
    }
}
