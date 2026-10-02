import QtQuick
import Qtui

Terminal {
    id: root
    width: 80
    height: 24
    backgroundColor: "#1E1E1E"

    FocusScope {
        anchors.fill: parent
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
                Qt.quit()
                event.accepted = true
            } else if (event.key === Qt.Key_O) {
                myPopup.open()
                statusText.text = "Status: Dialog Opened"
                event.accepted = true
            }
        }

        // Main content
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
            text: "<font color='cyan'>Qtui Popup & Z-Index Demo</font>"
            textFormat: Text.RichText
        }

        Text {
            id: statusText
            anchors.horizontalCenter: parent.horizontalCenter
            y: 7
            text: "Status: Ready. Press [O] to open Dialog."
            color: "gray"
        }

        // Button to open popup
        Text {
            id: openButton
            anchors.horizontalCenter: parent.horizontalCenter
            y: 10
            text: " [ Open Modal Dialog ] "
            color: "white"
            backgroundColor: "blue"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    myPopup.open()
                    statusText.text = "Status: Dialog Opened"
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 20
            text: "Press [Q] or [Esc] to exit."
            color: "dimgray"
        }
    }

    // The Popup element. Placed in hierarchy, but drawn on top!
    Popup {
        id: myPopup
        x: 20
        y: 6
        width: 40
        height: 10
        modal: true
        dimBackground: true

        // Draw dialog border & background
        Border {
            anchors.fill: parent
            style: "double"
            color: "yellow"
        }

        Item {
            anchors.fill: parent
            backgroundColor: "#2E2E2E"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 1
            text: "<b>WARNING</b>"
            textFormat: Text.RichText
            color: "red"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 3
            text: "This action cannot be undone."
            color: "white"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
            text: "Do you wish to continue?"
            color: "white"
        }

        // Confirm button
        Text {
            id: confirmButton
            x: 5
            y: 7
            text: " [ Confirm ] "
            color: "black"
            backgroundColor: "green"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    myPopup.close()
                    statusText.text = "Status: Confirmed!"
                }
            }
        }

        // Cancel button
        Text {
            id: cancelButton
            x: 23
            y: 7
            text: " [ Cancel ] "
            color: "black"
            backgroundColor: "red"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    myPopup.close()
                    statusText.text = "Status: Cancelled."
                }
            }
        }
    }
}
