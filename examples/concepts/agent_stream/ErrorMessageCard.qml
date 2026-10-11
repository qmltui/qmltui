import QtQml
import Qamui

Column {
    id: errCard
    required property string content
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "✖"
            color: "#f87171"
            bold: true
        }
        Text {
            text: " " + errCard.content
            color: "#fca5a5"
        }
    }
}
