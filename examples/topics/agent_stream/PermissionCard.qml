import QtQml
import QmlTui
import QmlTui.Controls

Column {
    id: permCard
    required property string content
    required property bool approved
    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        StatusBadge {
            text: permCard.approved ? "Approved" : "Denied"
            status: permCard.approved ? "success" : "error"
            variant: "subtle"
        }
        Text {
            text: " " + permCard.content
            color: "white"
        }
    }
}
