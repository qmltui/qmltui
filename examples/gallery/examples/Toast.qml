pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Toast: Stacked notification queue that auto-dismisses after a timeout.
Item {
    Column {
        spacing: 1

        Text {
            text: "Toasts stack in a corner and expire automatically."
            color: "#8b949e"
        }

        // Trigger notifications with different severity levels
        Row {
            spacing: 1

            Button {
                text: "Info"
                focus: true
                onClicked: toasts.showToast("Sync", "Background sync started", "info", 3000)
            }

            Button {
                text: "Success"
                onClicked: toasts.showToast("Saved", "All changes written", "success", 3000)
            }

            Button {
                text: "Warning"
                onClicked: toasts.showToast("Memory", "Heap above 80%", "warning", 3000)
            }

            Button {
                text: "Error"
                onClicked: toasts.showToast("Network", "Upstream unreachable", "error", 3000)
            }
        }
    }

    Toast {
        id: toasts
        anchors {
            bottom: parent.bottom
            right: parent.right
        }
        width: 30
        height: 10
        toastWidth: 28
        infoColor: "#7ee0ff"
        successColor: "#34d399"
        warningColor: "#fbbf24"
        errorColor: "#f87171"
    }
}
