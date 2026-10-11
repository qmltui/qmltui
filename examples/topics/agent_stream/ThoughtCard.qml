import QtQml
import Qamui

Column {
    id: thoughtCard
    required property string content
    required property string title

    property bool collapsed: false

    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: thoughtCard.collapsed ? "▶ Thought" : "▼ Thought"
            color: "#a78bfa"
            bold: true
        }
        Text {
            text: " (" + (thoughtCard.title || "0.8s") + ")"
            color: "#64748b"
        }
        Text {
            visible: thoughtCard.collapsed
            text: " · [Click to expand]"
            color: "#64748b"
        }
    }

    Text {
        visible: !thoughtCard.collapsed
        text: "  " + thoughtCard.content
        color: "#94a3b8"
        italic: true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: thoughtCard.collapsed = !thoughtCard.collapsed
    }
}
