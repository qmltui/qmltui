import QtQml 2.15
import Tui 1.0
import Tui.Controls 1.0

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

    Item {
        anchors.fill: parent
        backgroundColor: "#111111"

        // 1. Top Header - Anchored to top, left, and right of the parent
        Text {
            id: header
            height: 3
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 0
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            border.style: "single"
            border.color: "cyan"
            backgroundColor: "#222222"
            color: "cyan"
            bold: true
            text: "SYSTEM CONTROL DASHBOARD"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }

        // 2. Left Sidebar - Anchored top to header, left to parent, bottom to footer
        Column {
            id: sidebar
            width: 22
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.bottom: footer.top
            anchors.leftMargin: 0
            anchors.topMargin: 0
            anchors.bottomMargin: 0
            border.style: "single"
            border.color: "magenta"
            padding: 1
            spacing: 1

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "NAVIGATION"
                color: "magenta"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
            }

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "• Core Engine: OK"
                color: "green"
            }

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "• Mem Usage: 42%"
                color: "yellow"
            }

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "• Tasks: 3 Active"
                color: "silver"
            }
        }

        // 3. Bottom Footer - Anchored bottom, left, and right
        Text {
            id: footer
            height: 3
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 0
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            border.style: "single"
            border.color: "cyan"
            backgroundColor: "#151515"
            color: "gray"
            text: "Status: System Ready. Use Tab to cycle focus. Press 'q' to exit."
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            leftPadding: 2
        }

        // 4. Center Main Panel - Anchored to sidebar right, header bottom, parent right, footer top
        Column {
            id: mainPanel
            anchors.top: header.bottom
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.bottom: footer.top
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            anchors.topMargin: 0
            anchors.bottomMargin: 0
            border.style: "double"
            border.color: "green"
            padding: 1
            spacing: 1

            Text {
                width: parent.width - parent.leftPadding - parent.rightPadding
                text: "SYSTEM COMMAND INTERACTION CENTER"
                color: "green"
                bold: true
            }

            // Interactive Row (Button + TextInput)
            Row {
                width: parent.width - parent.leftPadding - parent.rightPadding
                height: 3
                spacing: 2

                TextInput {
                    id: cmdInput
                    width: 25
                    height: 3
                    border.style: activeFocus ? "double" : "single"
                    border.color: activeFocus ? "yellow" : "white"
                    text: "ping"
                    color: "white"

                    onAccepted: {
                        consoleResult.text = "Result: Command '" + text + "' executed.\nStatus code: 0\n[OK]"
                    }
                }

                Button {
                    label: "Run Task"
                    onClicked: {
                        consoleResult.text = "Result: Triggered task via '" + cmdInput.text + "'."
                    }
                }
            }

            // Command output console
            Text {
                id: consoleResult
                width: parent.width - parent.leftPadding - parent.rightPadding
                height: 4
                border.style: "single"
                border.color: "gray"
                backgroundColor: "#080808"
                color: "yellow"
                text: "Result: Waiting for command input..."
                padding: 1
            }

            // Interactive Mousearea coordinates display
            Text {
                id: mouseAreaInfo
                width: parent.width - parent.leftPadding - parent.rightPadding
                height: 3
                border.style: "single"
                border.color: "gray"
                text: "Move mouse inside panel to track coordinates.\nCoordinates: (" + panelMouseArea.mouseX + ", " + panelMouseArea.mouseY + ")"
                color: "cyan"
                padding: 1

                MouseArea {
                    id: panelMouseArea
                    width: parent.width
                    height: parent.height
                }
            }
        }
    }
}
