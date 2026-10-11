pragma ComponentBehavior: Bound

import QtQml
import Qamui

import Qamui.Controls
Popup {
    id: root

    property string dialogTitle: "Confirm Delete"
    property string message: "Are you sure you want to delete this item?"
    property string targetItem: ""

    signal dialogConfirmed()
    signal dialogRejected()

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: 48
    height: 8
    color: Theme.bgModal
    border.style: "double"
    border.color: Theme.accentRed

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Y || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.dialogConfirmed();
            root.close();
            event.accepted = true;
        } else if (event.key === Qt.Key_N || event.key === Qt.Key_Escape) {
            root.dialogRejected();
            root.close();
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⚠️  " + root.dialogTitle
            color: Theme.accentRed
            bold: true
        }

        Text {
            text: root.message
            color: Theme.textPrimary
        }

        Text {
            visible: root.targetItem !== ""
            text: "Target: " + root.targetItem
            color: Theme.accentYellow
            bold: true
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 3

            Text {
                text: "[y / Enter] Delete"
                color: Theme.accentRed
                bold: true
            }

            Text {
                text: "[n / Esc] Cancel"
                color: Theme.textSecondary
            }
        }
    }
}
