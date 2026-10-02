import QtQuick
import Terminal

Terminal {
    id: root
    width: 80
    height: 24
    color: "#1E1E1E"

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
    Rectangle {
        id: openButton
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        width: 24
        height: 1
        color: "blue"

        Text {
            anchors.fill: parent
            text: " [ Open Modal Dialog ] "
            color: "white"
            horizontalAlignment: Qt.AlignHCenter
        }

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

    // The Popup element. Placed in hierarchy, but drawn on top!
    Popup {
        id: myPopup
        anchors.centerIn: parent
        width: 40
        height: 10
        modal: true
        dimBackground: true
        color: "#2E2E2E"

        border {
            style: "double"
            color: "yellow"
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
        Rectangle {
            id: confirmButton
            x: 5
            y: 7
            width: 12
            height: 1
            color: "green"

            Text {
                anchors.fill: parent
                text: " [ Confirm ] "
                color: "black"
                horizontalAlignment: Qt.AlignHCenter
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    myPopup.close()
                    statusText.text = "Status: Confirmed!"
                }
            }
        }

        // Cancel button
        Rectangle {
            id: cancelButton
            x: 23
            y: 7
            width: 12
            height: 1
            color: "red"

            Text {
                anchors.fill: parent
                text: " [ Cancel ] "
                color: "black"
                horizontalAlignment: Qt.AlignHCenter
            }

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
