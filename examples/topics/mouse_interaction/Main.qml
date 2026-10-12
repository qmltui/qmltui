pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {

        id: rootWindow
        width: 80
        height: 24
        color: "green"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            // Header
            Rectangle {
                width: parent.width
                height: 3
                color: "#222222"

                Text {
                    anchors.fill: parent
                    text: "Concept Demo: Mouse Interactions & MouseArea"
                    color: "cyan"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
            }

            Text {
                width: parent.width
                text: "Move your mouse, click, or scroll the wheel inside the panel below.\nThe panel tracks coordinates, buttons, and wheel scroll natively."
                color: "gray"
            }

            // Mouse Tracker Panel
            Rectangle {
                id: mousePanel
                width: 50
                height: 8
                color: "#333333"

                Text {
                    id: mousePanelText
                    anchors.fill: parent
                    color: "white"
                    text: "Mouse Tracker Area\n\nCoordinates: (" + mouseArea.mouseX + ", " + mouseArea.mouseY + ")\nLast Event: None"
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true

                    onPressed: (event) => {
                        mousePanelText.text = "Mouse Tracker Area\n\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast Event: Pressed button " + event.button
                    }
                    onReleased: (event) => {
                        mousePanelText.text = "Mouse Tracker Area\n\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast Event: Released button " + event.button
                    }
                    onClicked: (event) => {
                        mousePanelText.text = "Mouse Tracker Area\n\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast Event: Clicked button " + event.button
                    }
                    onWheel: (event) => {
                        mousePanelText.text = "Mouse Tracker Area\n\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast Event: Wheel deltaY " + event.wheelDeltaY + " deltaX " + event.wheelDeltaX
                    }
                }
            }

            Text {
                width: parent.width
                text: "Press 'q' to exit this example."
                color: "silver"
            }
        }
    }
}
