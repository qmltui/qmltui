import QtQml 2.15
import Tui 1.0

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    width: 80
    height: 24
    backgroundColor: "blue"
    useAlternateScreen: true

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title Header
        Text {
            width: parent.width
            height: 3
            text: "Concept Demo: Layouts, Spacing & Padding"
            color: "yellow"
            backgroundColor: "#333333"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
        }

        // Padding Demo Section
        Text {
            width: parent.width
            text: "1. Nested Column with padding: 2 (offsets children by 2 units from edges)"
            color: "cyan"
        }

        Column {
            width: parent.width
            height: 7
            backgroundColor: "#222222"
            padding: 2
            spacing: 0

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "Inside padded container - Child Line 1 (x offset is 2)"
                color: "white"
                backgroundColor: "red"
            }

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "Inside padded container - Child Line 2"
                color: "white"
                backgroundColor: "green"
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

            Text {
                width: 20
                height: 3
                text: "Row Item 1"
                backgroundColor: "#444444"
                color: "white"
                horizontalAlignment: Qt.AlignHCenter
            }

            Text {
                width: 20
                height: 3
                text: "Row Item 2"
                backgroundColor: "#444444"
                color: "white"
                horizontalAlignment: Qt.AlignHCenter
            }

            Text {
                width: 20
                height: 3
                text: "Row Item 3"
                backgroundColor: "#444444"
                color: "white"
                horizontalAlignment: Qt.AlignHCenter
            }
        }

        Text {
            width: parent.width
            text: "Press 'q' to exit this example."
            color: "silver"
        }
    }
}
