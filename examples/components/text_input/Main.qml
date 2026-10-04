import QtQml 2.15
import Qtui

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "#0f172a"
    useAlternateScreen: true

    property string lastAction: "Ready for text input... Use Tab to cycle inputs"

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
                text: "Interactive TextInput Component Demo — Customizable Terminal Cursors"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Subtitle / Instructions
        Text {
            width: parent.width
            text: "Use Tab / Shift+Tab to switch inputs. Observe Block, Beam, Underline, and Custom Delegate cursors!"
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
                    text: "TERMINAL CURSOR STYLES DEMO"
                    color: "#f8fafc"
                    bold: true
                }

                // 1. Block Cursor (Cyan + Blinking)
                Row {
                    spacing: 2

                    Text {
                        width: 18
                        text: "1. Block (Blink):"
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

                            // Terminal Cursor Properties
                            cursorShape: TextInput.Block
                            cursorColor: "cyan"
                            cursorBlinking: true
                            cursorBlinkRate: 400

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Block: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Block: '" + text + "'"
                            }
                        }
                    }
                }

                // 2. Beam Cursor (Yellow)
                Row {
                    spacing: 2

                    Text {
                        width: 18
                        text: "2. Beam Shape:"
                        color: beamInput.activeFocus ? "#eab308" : "#94a3b8"
                        bold: beamInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: beamInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: beamInput.activeFocus ? "double" : "single"
                        border.color: beamInput.activeFocus ? "#eab308" : "#475569"

                        TextInput {
                            id: beamInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "operator@qtui.dev"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            // Terminal Cursor Properties
                            cursorShape: TextInput.Beam
                            cursorColor: "yellow"

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Beam: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Beam: '" + text + "'"
                            }
                        }
                    }
                }

                // 3. Underline Cursor (Green + Blinking)
                Row {
                    spacing: 2

                    Text {
                        width: 18
                        text: "3. Underline:"
                        color: cmdInput.activeFocus ? "#22c55e" : "#94a3b8"
                        bold: cmdInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: cmdInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: cmdInput.activeFocus ? "double" : "single"
                        border.color: cmdInput.activeFocus ? "#22c55e" : "#475569"

                        TextInput {
                            id: cmdInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "systemctl status --verbose"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            // Terminal Cursor Properties
                            cursorShape: TextInput.Underline
                            cursorColor: "green"
                            cursorBlinking: true
                            cursorBlinkRate: 350

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Underline: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Underline: '" + text + "'"
                            }
                        }
                    }
                }

                // 4. Custom Delegate Glyph Cursor (Magenta)
                Row {
                    spacing: 2

                    Text {
                        width: 18
                        text: "4. Custom (▋):"
                        color: customInput.activeFocus ? "#f43f5e" : "#94a3b8"
                        bold: customInput.activeFocus
                    }

                    Rectangle {
                        width: 45
                        height: 3
                        color: customInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: customInput.activeFocus ? "double" : "single"
                        border.color: customInput.activeFocus ? "#f43f5e" : "#475569"

                        TextInput {
                            id: customInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "custom glyph cursor"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            // Terminal Cursor Properties
                            cursorDelegate: "▋"
                            cursorColor: "#f43f5e"

                            onAccepted: {
                                rootWindow.lastAction = "Submitted Custom: '" + text + "'"
                            }
                            onTextChanged: {
                                rootWindow.lastAction = "Editing Custom: '" + text + "'"
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
