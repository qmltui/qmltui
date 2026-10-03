import QtQml
import Qtui

Popup {
    id: control

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
    border.color: activeFocus ? "yellow" : "white"
    color: "black"
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
            color: "yellow"
        }

        Text {
            text: control.message
            color: "white"
        }

        Row {
            spacing: 2

            Button {
                id: acceptButton
                label: control.acceptText
                onClicked: control.accept()
            }

            Button {
                visible: control.showRejectButton
                label: control.rejectText
                onClicked: control.reject()
            }
        }
    }
}
