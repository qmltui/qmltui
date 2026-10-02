import QtQml 2.15
import Qtui

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
    color: "green"
    useAlternateScreen: true

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title Header with background color and bold text
        Rectangle {
            width: parent.width
            height: 3
            color: "#222222"

            Text {
                anchors.fill: parent
                text: "Declarative Terminal UI Framework - Interactive Demo"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Row of focusable widgets
        Row {
            spacing: 4

            // Focusable Button 1
            Rectangle {
                id: opt1
                width: 15
                height: 3
                focusable: true
                color: activeFocus ? "blue" : "#111111"

                Text {
                    anchors.fill: parent
                    text: opt1.activeFocus ? ">> Option 1 <<" : "   Option 1   "
                    color: opt1.activeFocus ? "white" : "silver"
                    bold: opt1.activeFocus
                    verticalAlignment: Qt.AlignVCenter
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Return) {
                        statusText.text = "Selected Option 1 via Return Key!"
                        event.accepted = true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: (event) => {
                        opt1.focus = true
                        statusText.text = "Clicked Option 1!"
                    }
                }
            }

            // Focusable Button 2
            Rectangle {
                id: opt2
                width: 15
                height: 3
                focusable: true
                color: activeFocus ? "green" : "#111111"

                Text {
                    anchors.fill: parent
                    text: opt2.activeFocus ? ">> Option 2 <<" : "   Option 2   "
                    color: opt2.activeFocus ? "white" : "silver"
                    bold: opt2.activeFocus
                    verticalAlignment: Qt.AlignVCenter
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Return) {
                        statusText.text = "Selected Option 2 via Return Key!"
                        event.accepted = true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: (event) => {
                        opt2.focus = true
                        statusText.text = "Clicked Option 2!"
                    }
                }
            }
        }

        // Status Label
        Text {
            id: statusText
            width: parent.width
            text: "Use Tab/Shift-Tab to cycle focus, Enter to select, or Click any item."
            color: "yellow"
        }

        // Rich Text / ANSI parser showcase
        Column {
            spacing: 0
            
            Text {
                text: "ANSI Color & Attribute Parsing Showcase:"
                color: "gray"
                underline: true
            }

            Text {
                text: "\u001b[1;31mRed Bold\u001b[0m, \u001b[3mItalic Green\u001b[0m, \u001b[4;34mUnderline Blue\u001b[0m, \u001b[7;35mReversed Purple\u001b[0m"
            }

            Text {
                text: "Truecolor SGR: \u001b[38;2;255;165;0mOrange (255,165,0)\u001b[0m, \u001b[38;2;128;0;128;48;2;240;240;240mPurple on lightgray\u001b[0m"
            }
        }

        // Interactive Mouse Area Showcase
        Row {
            spacing: 2

            Rectangle {
                id: mouseDetector
                width: 40
                height: 5
                color: "#333333"

                Text {
                    id: mouseDetectorText
                    anchors.fill: parent
                    color: "white"
                    text: "Mouse Tracker Panel:\nMove mouse or click here\nCoordinates: (" + mouseArea.mouseX + ", " + mouseArea.mouseY + ")\nLast event: None"
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    
                    onPressed: (event) => {
                        mouseDetectorText.text = "Mouse Tracker Panel:\nMove mouse or click here\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast event: Pressed button " + event.button
                    }
                    onReleased: (event) => {
                        mouseDetectorText.text = "Mouse Tracker Panel:\nMove mouse or click here\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast event: Released button " + event.button
                    }
                    onClicked: (event) => {
                        mouseDetectorText.text = "Mouse Tracker Panel:\nMove mouse or click here\nCoordinates: (" + mouseX + ", " + mouseY + ")\nLast event: Clicked button " + event.button
                    }
                }
            }

            Text {
                width: parent.width - mouseDetector.width - 2
                height: 5
                text: "Instructions:\nPress 'q' to exit.\nResize the terminal window\nto see live resizing!"
                color: "red"
            }
        }
    }
}
