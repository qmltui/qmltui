import QtQml 2.15
import Terminal 1.0
import Terminal.Controls 1.0

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
    color: "black"
    useAlternateScreen: true

    Rectangle {
        anchors.fill: parent
        color: "#111111"

        // 1. Top Header - Anchored to top, left, and right of the parent
        Rectangle {
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
            color: "#222222"

            Text {
                anchors.fill: parent
                color: "cyan"
                bold: true
                text: "SYSTEM CONTROL DASHBOARD"
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // 2. Left Sidebar - Anchored top to header, left to parent, bottom to footer
        Rectangle {
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

            Column {
                anchors.fill: parent
                spacing: 1

                Text {
                    width: parent.width
                    text: "NAVIGATION"
                    color: "magenta"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: qsTr("• Core Engine: OK")
                    color: "green"
                }

                Text {
                    width: parent.width
                    text: qsTr("• Mem Usage: 42%")
                    color: "yellow"
                }

                Text {
                    width: parent.width
                    text: qsTr("• Tasks: 3 Active")
                    color: "silver"
                }
            }
        }

        // 3. Bottom Footer - Anchored bottom, left, and right
        Rectangle {
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
            color: "#151515"
            padding: 1

            Text {
                anchors.fill: parent
                color: "gray"
                text: "Status: System Ready. Use Tab to cycle focus. Press 'q' to exit."
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                leftPadding: 1
            }
        }

        // 4. Center Main Panel - Anchored to sidebar right, header bottom, parent right, footer top
        Rectangle {
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

            Column {
                anchors.fill: parent
                spacing: 1

                Text {
                    width: parent.width
                    text: "SYSTEM COMMAND INTERACTION CENTER"
                    color: "green"
                    bold: true
                }

                // Interactive Row (Button + TextInput)
                Row {
                    width: parent.width
                    height: 3
                    spacing: 2

                    Rectangle {
                        width: 25
                        height: 3
                        border.style: cmdInput.activeFocus ? "double" : "single"
                        border.color: cmdInput.activeFocus ? "yellow" : "white"

                        TextInput {
                            id: cmdInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "ping"
                            color: "white"

                            onAccepted: {
                                consoleResultText.text = "Result: Command '" + text + "' executed.\nStatus code: 0\n[OK]"
                            }
                        }
                    }

                    Button {
                        label: "Run Task"
                        onClicked: {
                            consoleResultText.text = "Result: Triggered task via '" + cmdInput.text + "'."
                        }
                    }
                }

                // Command output console
                Rectangle {
                    id: consoleResult
                    width: parent.width
                    height: 4
                    border.style: "single"
                    border.color: "gray"
                    color: "#080808"
                    padding: 1

                    Text {
                        id: consoleResultText
                        anchors.fill: parent
                        color: "yellow"
                        text: "Result: Waiting for command input..."
                    }
                }

                // Interactive Mousearea coordinates display
                Rectangle {
                    id: mouseAreaInfo
                    width: parent.width
                    height: 3
                    border.style: "single"
                    border.color: "gray"
                    padding: 1

                    Text {
                        anchors.fill: parent
                        text: "Move mouse inside panel to track coordinates.\nCoordinates: (" + panelMouseArea.mouseX + ", " + panelMouseArea.mouseY + ")"
                        color: "cyan"
                    }

                    MouseArea {
                        id: panelMouseArea
                        anchors.fill: parent
                    }
                }
            }
        }
    }
}
