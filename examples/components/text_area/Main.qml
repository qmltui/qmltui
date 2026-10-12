import QtQml
import Qamui

Terminal {
    id: rootWindow
    color: "#0f172a"

    property string lastAction: "Ready. Focus a TextArea using Tab/Shift+Tab and start editing."

    // Key handling to quit application as required by project rules
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
            border.style: Border.Single
            border.color: "cyan"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "Interactive TextArea Component Demo"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Subtitle & Instructions
        Text {
            width: parent.width
            text: "Tab / Shift+Tab: Change Focus | Arrows / Home / End: Move Cursor | Enter: New Line | Esc / Q: Quit"
            color: "#94a3b8"
        }

        // Section 1: Editable Multi-Line Text Editor
        Rectangle {
            width: parent.width
            height: 12
            color: "#020617"
            border.style: editableArea.activeFocus ? Border.Double : Border.Single
            border.color: editableArea.activeFocus ? "#38bdf8" : "#334155"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Row {
                    spacing: 2
                    Text {
                        text: " [1] EDITABLE NOTEBOOK "
                        color: editableArea.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: true
                    }
                    Text {
                        text: editableArea.activeFocus ? "(Active Focus)" : "(Press Tab to Focus)"
                        color: editableArea.activeFocus ? "#22c55e" : "#64748b"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 8
                    color: editableArea.activeFocus ? "#0f172a" : "#020617"
                    border.style: Border.Single
                    border.color: editableArea.activeFocus ? "#0284c7" : "#1e293b"

                    TextArea {
                        id: editableArea
                        anchors.fill: parent
                        anchors.margins: 1
                        focusable: true
                        placeholderText: "Type your notes or multi-line text here..."
                        color: "#f8fafc"
                        text: "Welcome to Qamui TextArea!\nThis control supports multi-line text editing in terminal UI.\nFeel free to type, add line breaks with Enter, or move around with arrow keys."

                        onTextChanged: {
                            rootWindow.lastAction = "Editable TextArea updated (" + text.length + " chars)"
                        }

                        onCursorPositionChanged: {
                            cursorInfoText.text = "Pos: " + cursorPosition + " | Sel: [" + selectionStart + ".." + selectionEnd + "]"
                        }
                    }
                }
            }
        }

        // Section 2: Metadata & Status Bar
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: Border.Single
            border.color: "#475569"

            Row {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 4

                Text {
                    id: cursorInfoText
                    text: "Pos: " + editableArea.cursorPosition + " | Sel: [" + editableArea.selectionStart + ".." + editableArea.selectionEnd + "]"
                    color: "#f59e0b"
                    bold: true
                }

                Text {
                    text: "Total Chars: " + editableArea.text.length
                    color: "#a855f7"
                }

                Text {
                    text: "Read-Only: " + (readOnlyArea.readOnly ? "Yes" : "No")
                    color: "#06b6d4"
                }
            }
        }

        // Section 3: Read-Only Console Log / Output View
        Rectangle {
            width: parent.width
            height: parent.height - 23
            color: "#020617"
            border.style: readOnlyArea.activeFocus ? Border.Double : Border.Single
            border.color: readOnlyArea.activeFocus ? "#38bdf8" : "#334155"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Row {
                    spacing: 2
                    Text {
                        text: " [2] SYSTEM CONFIG PREVIEW (READ-ONLY) "
                        color: readOnlyArea.activeFocus ? "#38bdf8" : "#94a3b8"
                        bold: true
                    }
                    Text {
                        text: readOnlyArea.activeFocus ? "(Active Focus - Scrollable)" : "(Unfocused)"
                        color: readOnlyArea.activeFocus ? "#22c55e" : "#64748b"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: parent.height - 3
                    color: readOnlyArea.activeFocus ? "#0f172a" : "#020617"
                    border.style: Border.Single
                    border.color: readOnlyArea.activeFocus ? "#0284c7" : "#1e293b"

                    TextArea {
                        id: readOnlyArea
                        anchors.fill: parent
                        anchors.margins: 1
                        focusable: true
                        readOnly: true
                        color: "#38bdf8"
                        text: "# System Configuration File\n" +
                              "server_name: \"qamui-terminal-demo\"\n" +
                              "max_connections: 1024\n" +
                              "timeout_ms: 5000\n" +
                              "render_engine: \"TuiTextGridBuffer\"\n" +
                              "status: \"Active and listening...\""
                    }
                }
            }
        }

        // Status Bar Footer
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: Border.Single
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
