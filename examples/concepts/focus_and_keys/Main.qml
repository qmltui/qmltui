import QtQml
import Qtui

Terminal {
    id: rootWindow

    color: "black"

    // 1. Declarative Shortcuts (Feature Showcase)
    Shortcut {
        sequence: Qt.Key_Q
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: Qt.Key_Escape
        onActivated: Qt.quit()
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Header
        Rectangle {
            width: parent.width
            height: 3
            color: "#222222"

            Text {
                anchors.fill: parent
                text: "Showcase: New Key Handling Features (Shortcuts, Specific Keys, Priority, Forwarding)"
                color: "yellow"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        Text {
            width: parent.width
            text: "Use Tab/Shift-Tab to cycle focus. Press 'q' or 'Escape' to quit."
                  + (rootWindow.capabilities.kittyKeyboard
                     ? "  Keyboard: kitty protocol (Ctrl+Enter and Shift+Enter are distinct)."
                     : "  Keyboard: legacy (Ctrl+Enter and Shift+Enter arrive as Enter).")
            color: "gray"
        }

        // --- Row of Showcase Features ---
        Row {
            spacing: 2
            width: parent.width
            height: 11

            // Feature A: Specific Key Signals
            Rectangle {
                id: boxSpecificKeys
                width: 25
                height: 10
                focusable: true
                color: activeFocus ? "#1a365d" : "#111111"
                border.color: activeFocus ? "blue" : "#333333"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 1

                    Text {
                        text: boxSpecificKeys.activeFocus ? "[*] 1. Specific Keys" : "    1. Specific Keys"
                        color: "white"
                        bold: true
                    }
                    Text {
                        text: "Press Return or Space\nto test specific signals.\n"
                              + "Ctrl+Enter and Shift+Enter\nare told apart on kitty,\n"
                              + "Ghostty, WezTerm and foot."
                        color: "silver"
                    }
                }

                // With the kitty keyboard protocol the modifiers survive; a
                // legacy terminal sends the same bytes for all three chords
                // and this handler sees a plain Return. Ghostty binds
                // Ctrl+Enter to toggle_fullscreen by default, so the key never
                // reaches the app there unless that binding is removed.
                Keys.onReturnPressed: (event) => {
                    const chord = (event.modifiers & Qt.ControlModifier) ? "Ctrl+Enter"
                                : (event.modifiers & Qt.ShiftModifier) ? "Shift+Enter"
                                : "Enter"
                    logText.text = "Keys.onReturnPressed on Item A: " + chord
                    event.accepted = true
                }
                Keys.onSpacePressed: (event) => {
                    logText.text = "Keys.onSpacePressed triggered on Item A!"
                    event.accepted = true
                }
            }

            // Feature B: Event Forwarding (Keys.forwardTo)
            Rectangle {
                id: boxForwardingParent
                width: 25
                height: 10
                focusable: true
                color: activeFocus ? "#1c3d2f" : "#111111"
                border.color: activeFocus ? "green" : "#333333"

                // Forward keys to the child item inside
                Keys.forwardTo: [boxForwardingChild]

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 1

                    Text {
                        text: boxForwardingParent.activeFocus ? "[*] 2. Forwarding" : "    2. Forwarding"
                        color: "white"
                        bold: true
                    }
                    Text {
                        text: "Type any key. Parent\nforwards to child."
                        color: "silver"
                    }

                    // Child status display
                    Rectangle {
                        id: boxForwardingChild
                        width: 23
                        height: 3
                        color: "#222222"
                        border.color: "gray"

                        Text {
                            anchors.fill: parent
                            text: "Child status label"
                            id: childStatusText
                            color: "green"
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                        }

                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab)
                                return;

                            childStatusText.text = "Fwd: " + event.text
                            logText.text = "Child received forwarded key: " + event.text
                            event.accepted = true
                        }
                    }
                }
            }

            // Feature C: Priority Interception (Keys.priority)
            Rectangle {
                id: boxPriority
                width: 26
                height: 10
                color: "#111111"
                border.color: "#333333"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 1

                    Text {
                        text: " 3. Priority Routing"
                        color: "white"
                        bold: true
                    }

                    Text {
                        text: "Intercept Left Arrow:"
                        color: "silver"
                    }

                    TextInput {
                        id: inputPriority
                        width: 24
                        height: 1
                        text: "Press Left Arrow"
                        color: "yellow"
                        focusable: true

                        // Intercept Left key before the C++ TextInput handles it
                        Keys.priority: Keys.BeforeItem
                        Keys.onLeftPressed: (event) => {
                            logText.text = "QML intercepted Left Arrow BEFORE C++ TextInput!"
                            event.accepted = true // blocks cursor movement
                        }
                    }
                    
                    Text {
                        text: "(Left is blocked by QML\nBeforeItem priority)"
                        color: "gray"
                    }
                }
            }
        }

        // Status / Log Box
        Rectangle {
            width: parent.width
            height: 5
            color: "#111111"
            border.color: "#222222"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    text: "Interaction Log:"
                    color: "gray"
                }
                Text {
                    id: logText
                    text: "Focus an item and press keys to begin."
                    color: "cyan"
                }
            }
        }
    }
}
