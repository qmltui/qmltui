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
    backgroundColor: "black"
    useAlternateScreen: true

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Header
        Text {
            width: parent.width
            height: 3
            text: "Concept Demo: Text Styling & ANSI Parser"
            color: "yellow"
            backgroundColor: "#222222"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }

        // Section 1: Explicit QML properties
        Text {
            width: parent.width
            text: "1. Explicit Text Properties:"
            color: "cyan"
            underline: true
        }

        Row {
            spacing: 2
            width: parent.width

            Text {
                width: 15
                text: "Bold Text"
                color: "green"
                bold: true
            }

            Text {
                width: 15
                text: "Italic Text"
                color: "green"
                italic: true
            }

            Text {
                width: 18
                text: "Underline Text"
                color: "green"
                underline: true
            }

            Text {
                width: 15
                text: "Reversed Text"
                color: "green"
                reversed: true
            }
        }

        // Section 2: ANSI escape sequences
        Text {
            width: parent.width
            text: "\n2. Inline ANSI Escape Sequence Parsing:"
            color: "cyan"
            underline: true
        }

        Text {
            width: parent.width
            text: "16-Color: \u001b[1;31mBold Red\u001b[0m, \u001b[4;32mUnderline Green\u001b[0m, \u001b[7;33mReversed Yellow\u001b[0m"
        }

        Text {
            width: parent.width
            text: "Truecolor SGR: \u001b[38;2;255;165;0mOrange Foreground (255, 165, 0)\u001b[0m"
        }

        Text {
            width: parent.width
            text: "Background SGR: \u001b[48;2;128;0;128;38;2;255;255;255mWhite text on Purple Background\u001b[0m"
        }

        Text {
            width: parent.width
            text: "\nPress 'q' to exit this example."
            color: "silver"
        }
    }
}
