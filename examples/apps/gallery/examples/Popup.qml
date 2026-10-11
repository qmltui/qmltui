pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// Popup: Free-form modal overlay. One tile of the Qamui gallery (examples/apps/gallery)
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
        id: popupDemo
        anchors.fill: parent
        property int opened: 0
        Column {
            spacing: 1
            Text { text: "Popups draw above everything else; modal ones dim the frame."; color: "#6e7b8b" }
            Button { text: "Open sheet"; focus: true; onClicked: sheet.open() }
            Text { text: "Opened " + popupDemo.opened + " time" + (popupDemo.opened === 1 ? "" : "s"); color: "#a3e635" }
        }
        Popup {
            id: sheet
            anchors.centerIn: parent
            width: 44
            height: 10
            modal: true
            color: "#141b24"
            border.style: "double"
            border.color: "#a3e635"
            onOpened: popupDemo.opened++
            Text { x: 2; y: 1; text: "Export options"; bold: true; color: "#a3e635" }
            CheckBox { id: includeImages; x: 2; y: 3; text: "Include images"; checked: true }
            CheckBox { x: 2; y: 4; text: "Compress output" }
            Row {
                x: 2; y: 6
                spacing: 2
                Button { text: "Export"; onClicked: sheet.accept() }
                Button { text: "Cancel"; onClicked: sheet.reject() }
            }
        }
    }
}
