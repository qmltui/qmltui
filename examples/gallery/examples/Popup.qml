pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Popup: Free-form modal overlay surface with backdrop dimming and focus trapping.
Item {
    id: popupDemo

    property int opened: 0

    Column {
        spacing: 1

        Text {
            text: "Popups render in the overlay layer; modal: true dims the background."
            color: "#8b949e"
        }

        Button {
            text: "Open sheet"
            focus: true
            onClicked: sheet.open()
        }

        Text {
            text: "Opened " + popupDemo.opened + " time" + (popupDemo.opened === 1 ? "" : "s")
            color: "#a3e635"
        }
    }

    Popup {
        id: sheet
        anchors.centerIn: parent
        width: 40
        height: 9
        modal: true
        onOpened: popupDemo.opened++

        Rectangle {
            anchors.fill: parent
            color: "#161b22"
            border {
                style: Border.Double
                color: "#a3e635"
            }
        }

        Text {
            x: 2
            y: 1
            text: "Export options"
            bold: true
            color: "#a3e635"
        }

        CheckBox {
            x: 2
            y: 3
            text: "Include images"
            checked: true
        }

        CheckBox {
            x: 2
            y: 4
            text: "Compress output"
        }

        Row {
            x: 2
            y: 6
            spacing: 2

            Button {
                text: "Export"
                onClicked: sheet.accept()
            }

            Button {
                text: "Cancel"
                onClicked: sheet.reject()
            }
        }
    }
}
