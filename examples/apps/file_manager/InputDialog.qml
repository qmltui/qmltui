pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls

Popup {
    id: root

    property string dialogTitle: "New Item"
    property string promptText: "Enter name:"
    property string inputValue: ""
    property bool isDirectory: false
    property bool allowToggleDir: true

    signal dialogAccepted(string name, bool isDir)
    signal dialogRejected()

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: 48
    height: 9
    color: Theme.bgModal
    border.style: "double"
    border.color: Theme.accentCyan

    onOpened: {
        textInput.text = root.inputValue;
        textInput.focus = true;
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.dialogAccepted(textInput.text, root.isDirectory);
            root.close();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            root.dialogRejected();
            root.close();
            event.accepted = true;
        } else if (event.key === Qt.Key_Tab && root.allowToggleDir) {
            root.isDirectory = !root.isDirectory;
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.dialogTitle
            color: Theme.accentCyan
            bold: true
        }

        Text {
            text: root.promptText
            color: Theme.textSecondary
        }

        // Text input field
        Rectangle {
            width: parent.width - 2
            height: 1
            color: Theme.bgCursor
            border.style: "none"

            TextInput {
                id: textInput
                anchors.fill: parent
                text: root.inputValue
                color: Theme.textHighlight
                cursorVisible: true
                cursorColor: Theme.accentYellow
                onAccepted: {
                    root.accepted(textInput.text, root.isDirectory);
                    root.close();
                }
            }
        }

        // Toggle Type (if allowed)
        Row {
            visible: root.allowToggleDir
            spacing: 1

            Text {
                text: "Type [Tab]: "
                color: Theme.textDim
            }

            Text {
                text: root.isDirectory ? "📁 Directory" : "📄 File"
                color: root.isDirectory ? Theme.accentBlue : Theme.accentGreen
                bold: true
            }
        }

        // Controls hint
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 2

            Text {
                text: "[Enter] Confirm"
                color: Theme.accentGreen
                bold: true
            }

            Text {
                text: "[Esc] Cancel"
                color: Theme.textMuted
            }
        }
    }
}
