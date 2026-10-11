pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// Toast: Timed notifications. One tile of the Qamui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Item {
        anchors.fill: parent
        Column {
            spacing: 1
            Text { text: "Toasts stack in a corner and expire on their own."; color: "#6e7b8b" }
            Row {
                spacing: 2
                Button { text: "Info"; focus: true; onClicked: toasts.showToast("Sync", "Background sync started", "info", 3000) }
                Button { text: "Success"; onClicked: toasts.showToast("Saved", "All changes written", "success", 3000) }
                Button { text: "Warning"; onClicked: toasts.showToast("Memory", "Heap above 80%", "warning", 3000) }
                Button { text: "Error"; onClicked: toasts.showToast("Network", "Upstream unreachable", "error", 3000) }
            }
        }
        Toast {
            id: toasts
            anchors.top: parent.top
            anchors.right: parent.right
            width: 32
            height: 12
            toastWidth: 30
            infoColor: "#7ee0ff"
            successColor: "#34d399"
            warningColor: "#fbbf24"
            errorColor: "#f87171"
        }
    }
}
