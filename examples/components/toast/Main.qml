import QtQml
import Qamui

import Qamui.Controls

Terminal {

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "🔔 TOASTMANAGER NOTIFICATIONS DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Click buttons to trigger overlay notifications. Press Q/ESC to quit"
            color: "gray"
        }

        Row {
            spacing: 2

            Button {
                text: "Info"
                onClicked: toastMgr.showToast("System Info", "Background sync in progress", "info", 3000)
            }

            Button {
                text: "Success"
                onClicked: toastMgr.showToast("Success", "Build completed cleanly!", "success", 3000)
            }

            Button {
                text: "Warning"
                onClicked: toastMgr.showToast("Warning", "High memory usage detected", "warning", 3000)
            }

            Button {
                text: "Error"
                onClicked: toastMgr.showToast("Connection Error", "Failed to reach backend", "error", 3000)
            }
        }
    }

    Toast {
        id: toastMgr
        anchors.top: parent.top
        anchors.right: parent.right
        width: 32
        height: 16
    }

    Component.onCompleted: {
        toastMgr.showToast("Welcome", "Click any button to trigger a toast!", "info", 4000)
    }
}
