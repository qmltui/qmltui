import QtQuick
import Qtui

Column {
    id: toolResultCard
    required property string content
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "  └─"
            color: "#64748b"
        }
        Text {
            text: " " + toolResultCard.content
            color: "#34d399"
        }
    }
}
