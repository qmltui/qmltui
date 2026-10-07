import QtQml
import Qtui

Popup {
    id: control

    property rgba textColor: "default"
    property rgba titleColor: textColor
    property string title: ""
    property string message: ""
    property string acceptText: qsTr("OK")
    property string rejectText: qsTr("Cancel")
    property bool showRejectButton: true

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    implicitWidth: Math.max(32, contentColumn.implicitWidth + 4)
    implicitHeight: contentColumn.implicitHeight + 2
    border.style: "double"
    border.color: "default"
    color: "default"
    initialFocusItem: acceptButton
    anchors.centerIn: parent

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            acceptButton.clicked()
            event.accepted = true
        }
    }

    Column {
        id: contentColumn
        x: 2
        y: 1
        spacing: 1

        Text {
            text: control.title
            bold: true
            color: control.titleColor
        }

        Text {
            text: control.message
            color: control.textColor
        }

        Row {
            spacing: 2

            Button {
                id: acceptButton
                text: control.acceptText
                onClicked: control.accept()
            }

            Button {
                visible: control.showRejectButton
                text: control.rejectText
                onClicked: control.reject()
            }
        }
    }
}
