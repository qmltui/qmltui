pragma ComponentBehavior: Bound
import QtQml
import Qtui

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
        color: "blue"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            // Title Header
            Rectangle {
                width: parent.width
                height: 3
                color: "#333333"

                Text {
                    anchors.fill: parent
                    text: "Concept Demo: Layouts, Spacing & Padding"
                    color: "yellow"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
            }

            // Padding Demo Section
            Text {
                width: parent.width
                text: "1. Nested Column with padding: 2 (offsets children by 2 units from edges)"
                color: "cyan"
            }

            Rectangle {
                width: parent.width
                height: 7
                color: "#222222"
                padding: 2

                Column {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "red"
                        Text {
                            anchors.fill: parent
                            text: "Inside padded container - Child Line 1 (x offset is 2)"
                            color: "white"
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "green"
                        Text {
                            anchors.fill: parent
                            text: "Inside padded container - Child Line 2"
                            color: "white"
                        }
                    }
                }
            }

            // Spacing Demo Section
            Text {
                width: parent.width
                text: "2. Row Layout with spacing: 4"
                color: "cyan"
            }

            Row {
                spacing: 4
                width: parent.width

                Rectangle {
                    width: 20
                    height: 3
                    color: "#444444"
                    Text {
                        anchors.fill: parent
                        text: "Row Item 1"
                        color: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }
                }

                Rectangle {
                    width: 20
                    height: 3
                    color: "#444444"
                    Text {
                        anchors.fill: parent
                        text: "Row Item 2"
                        color: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }
                }

                Rectangle {
                    width: 20
                    height: 3
                    color: "#444444"
                    Text {
                        anchors.fill: parent
                        text: "Row Item 3"
                        color: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
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
