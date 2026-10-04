import QtQuick
import Qtui

Column {
    id: userCard
    required property string content
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "❯"
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " " + userCard.content
            color: "white"
            bold: true
        }
    }
}
