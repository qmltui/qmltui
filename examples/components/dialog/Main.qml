import QtQml 2.15
import Qtui
import Qtui.Controls

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Qtui Dialog Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Button {
            label: "[ Open Dialog ]"
            onClicked: confirmDlg.open()
        }

        Text {
            id: statusText
            text: "Status: Ready."
            color: "yellow"
        }
    }

    Dialog {
        id: confirmDlg
        title: "Confirm Action"
        message: "Are you sure you want to delete the file?"
        acceptText: "Delete"
        rejectText: "Cancel"
        onAccepted: statusText.text = "Action Confirmed!"
        onRejected: statusText.text = "Action Cancelled."
    }
}
