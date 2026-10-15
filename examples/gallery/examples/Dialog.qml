pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Dialog: Modal accept/reject. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Column {
        id: dialogDemo
        x: 2
        y: 1
        property string outcome: "No decision yet"
        spacing: 1
        Text {
            text: "A Dialog is a Popup with a title, a message and two buttons."
            color: "#8b949e"
        }
        Button {
            text: "Delete report.pdf"
            focus: true
            onClicked: confirmDialog.open()
        }
        Text {
            text: dialogDemo.outcome
            color: "#f87171"
            bold: true
        }
        Dialog {
            id: confirmDialog
            title: "Delete file"
            message: "report.pdf will be removed. This cannot be undone."
            acceptText: "Delete"
            rejectText: "Keep"
            onAccepted: dialogDemo.outcome = "Deleted report.pdf"
            onRejected: dialogDemo.outcome = "Kept report.pdf"
        }
    }
}
