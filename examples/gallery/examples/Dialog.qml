pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Dialog: Modal accept/reject confirmation dialog with contextual Escape.
Item {
    id: dialogDemo

    property string outcome: "No decision yet"

    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Dialog provides a modal prompt with accept and reject buttons."
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
    }

    // Enter accepts; Escape rejects and closes the Dialog before reaching root
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
