import QtQml 2.15
import Qtui

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "#0f172a"
    useAlternateScreen: true

    property string lastAction: "Ready for text input..."

    // Explicit key handling to quit, as per project rules (.agents/AGENTS.md)
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title Header
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "cyan"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "Interactive TextInput Component Demo"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Subtitle / Instructions
        Text {
            width: parent.width
            text: "Use Tab / Shift+Tab to switch focus. Type to edit. Left/Right/Home/End/Backspace/Delete work natively. Press Enter to submit."
            color: "#94a3b8"
        }

        // Main Input Form Panel
        Rectangle {
            width: parent.width
            height: parent.height - 8
            color: "#020617"
            border.style: "double"
            border.color: "#334155"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    text: "SYSTEM LOGIN & COMMAND FORM"
                    color: "#f8fafc"
                    bold: true
                }

                // 1. Username Field
                Row {
                    spacing: 2

                    Text {
                        width: 14
                        text: "Username:"
                        color: userInput.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: userInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: userInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: userInput.activeFocus ? "double" : "single"
                        border.color: userInput.activeFocus ? "#38bdf8" : "#475569"

                        TextInput {
                            id: userInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "admin"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Username: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Username: '" + text + "'"
                            }
                        }
                    }
                }

                // 2. Email Field
                Row {
                    spacing: 2

                    Text {
                        width: 14
                        text: "Email:"
                        color: emailInput.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: emailInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: emailInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: emailInput.activeFocus ? "double" : "single"
                        border.color: emailInput.activeFocus ? "#38bdf8" : "#475569"

                        TextInput {
                            id: emailInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "operator@qtui.dev"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Email: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Email: '" + text + "'"
                            }
                        }
                    }
                }

                // 3. Terminal Command Line Field
                Row {
                    spacing: 2

                    Text {
                        width: 14
                        text: "Command:"
                        color: cmdInput.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: cmdInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: cmdInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: cmdInput.activeFocus ? "double" : "single"
                        border.color: cmdInput.activeFocus ? "#38bdf8" : "#475569"

                        TextInput {
                            id: cmdInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "systemctl status --verbose"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            onAccepted: {
                                rootWindow.lastAction = "Executed Command: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Command: '" + text + "'"
                            }
                        }
                    }
                }
            }
        }

        // Live Action / Output Bar
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "yellow"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "STATUS: " + rootWindow.lastAction
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
