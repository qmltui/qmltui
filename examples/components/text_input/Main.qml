import QtQml
import Qtui

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "#0f172a"
    useAlternateScreen: true

    property string lastAction: "Ready for text input... Tab: cycle | Ins: Overwrite mode | Shift+Arrows: select"

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
                text: "TextInput Component Demo — GridLayout & Terminal Features"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Subtitle / Instructions
        Text {
            width: parent.width
            text: "Tab: next | Ins: toggle Overwrite | Up/Down: Scroll | Shift+Arrows: select"
            color: "#94a3b8"
        }

        // Main Input Form Container wrapped in ScrollView with GridLayout
        Rectangle {
            width: parent.width
            height: parent.height - 8
            color: "#020617"
            border.style: "double"
            border.color: "#334155"

            ScrollView {
                anchors.fill: parent
                anchors.margins: 1
                focusable: true

                GridLayout {
                    width: parent.width - 2
                    columns: 2
                    columnSpacing: 2
                    rowSpacing: 1

                    // Header row spanning 2 columns
                    Text {
                        text: "TERMINAL INPUT FIELDS (GRIDLAYOUT)"
                        color: "#f8fafc"
                        bold: true
                        Layout.columnSpan: 2
                    }

                    // 1. Block Cursor (Cyan + Blinking)
                    Text {
                        text: "1. Block (Blink):"
                        color: userInput.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: userInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
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

                            onAccepted: rootWindow.lastAction = "Submitted Block: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Block: '" + text + "'"
                        }
                    }

                    // 2. Beam Cursor (Yellow)
                    Text {
                        text: "2. Beam Shape:"
                        color: beamInput.activeFocus ? "#eab308" : "#94a3b8"
                        bold: beamInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
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

                            onAccepted: rootWindow.lastAction = "Submitted Beam: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Beam: '" + text + "'"
                        }
                    }

                    // 3. Underline Cursor (Green + Blinking)
                    Text {
                        text: "3. Underline:"
                        color: cmdInput.activeFocus ? "#22c55e" : "#94a3b8"
                        bold: cmdInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
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

                            onAccepted: rootWindow.lastAction = "Submitted Underline: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Underline: '" + text + "'"
                        }
                    }

                    // 4. Custom Delegate Glyph Cursor (Magenta)
                    Text {
                        text: "4. Custom Glyph (▋):"
                        color: customInput.activeFocus ? "#f43f5e" : "#94a3b8"
                        bold: customInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
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

                            onAccepted: rootWindow.lastAction = "Submitted Custom: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Custom: '" + text + "'"
                        }
                    }

                    // 5. Custom Selection Colors (Blue Background + Yellow Text)
                    Text {
                        text: "5. Custom Selection:"
                        color: selInput.activeFocus ? "#a855f7" : "#94a3b8"
                        bold: selInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
                        color: selInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: selInput.activeFocus ? "double" : "single"
                        border.color: selInput.activeFocus ? "#a855f7" : "#475569"

                        TextInput {
                            id: selInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "Shift+Arrow to select this text"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            // Custom Selection & Cursor Styling
                            cursorShape: TextInput.Beam
                            cursorColor: "#a855f7"
                            selectionColor: "#1e3a8a"
                            selectedTextColor: "#fde047"

                            onAccepted: rootWindow.lastAction = "Submitted Selection: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Selection: '" + text + "'"
                        }
                    }

                    // 6. Placeholder Color & Clear
                    Text {
                        text: "6. Placeholder Color:"
                        color: holderInput.activeFocus ? "#ec4899" : "#94a3b8"
                        bold: holderInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
                        color: holderInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: holderInput.activeFocus ? "double" : "single"
                        border.color: holderInput.activeFocus ? "#ec4899" : "#475569"

                        TextInput {
                            id: holderInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: ""
                            placeholderText: "Type custom query here... (Pink placeholder)"
                            placeholderColor: "#ec4899"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true

                            cursorShape: TextInput.Underline
                            cursorColor: "#ec4899"

                            onAccepted: rootWindow.lastAction = "Submitted Search: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Search: '" + text + "'"
                        }
                    }

                    // 7. Overwrite Mode (Insert key toggleable)
                    Text {
                        text: "7. Overwrite (Ins):"
                        color: overwriteInput.activeFocus ? "#10b981" : "#94a3b8"
                        bold: overwriteInput.activeFocus
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Rectangle {
                        implicitWidth: 45
                        implicitHeight: 3
                        color: overwriteInput.activeFocus ? "#1e293b" : "#0f172a"
                        border.style: overwriteInput.activeFocus ? "double" : "single"
                        border.color: overwriteInput.activeFocus ? "#10b981" : "#475569"

                        TextInput {
                            id: overwriteInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "Type over these characters"
                            color: activeFocus ? "#ffffff" : "#cbd5e1"
                            focusable: true
                            overwriteMode: true

                            cursorShape: TextInput.Block
                            cursorColor: "#10b981"

                            onOverwriteModeChanged: rootWindow.lastAction = "Overwrite Mode: " + (overwriteMode ? "ON" : "OFF")
                            onAccepted: rootWindow.lastAction = "Submitted Overwrite: '" + text + "'"
                            onTextChanged: rootWindow.lastAction = "Editing Overwrite: '" + text + "' (displayText: '" + displayText + "')"
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
