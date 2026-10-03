import QtQml 2.15
import Qtui
import Qtui.Controls
import Qtui.TerminalHost

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    color: "#0d0e15"

    // Selected target for command dispatcher: 0 = Term 1, 1 = Term 2, 2 = Both (Broadcast)
    property int commandTarget: 0
    property string logMessage: "System initialized. Press [F1]/[F2] for focus, [F3] toggle, [Esc/F4] unfocus."

    function quitApp() {
        terminal1.kill()
        terminal2.kill()
        Qt.quit()
    }

    function toggleTerminalFocus() {
        if (terminal1.activeFocus) {
            terminal2.focus = true
            commandTarget = 1
            logMessage = "Focus toggled to Terminal 2 [F3]"
        } else {
            terminal1.focus = true
            commandTarget = 0
            logMessage = "Focus toggled to Terminal 1 [F3]"
        }
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: (event) => {
        if ((event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier)) || event.key === Qt.Key_Escape) {
            quitApp()
            event.accepted = true
        } else if (event.key === Qt.Key_F1) {
            terminal1.focus = true
            commandTarget = 0
            logMessage = "Focus switched to Terminal 1 [F1]"
            event.accepted = true
        } else if (event.key === Qt.Key_F2) {
            terminal2.focus = true
            commandTarget = 1
            logMessage = "Focus switched to Terminal 2 [F2]"
            event.accepted = true
        } else if (event.key === Qt.Key_F3) {
            toggleTerminalFocus()
            event.accepted = true
        } else if (event.key === Qt.Key_F4) {
            cmdInput.focus = true
            logMessage = "Unfocused terminals -> Focused Command Input [F4]"
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // =============================================================
        // 1. SLEEK TOP HEADER BAR (Height: 2)
        // =============================================================
        Rectangle {
            width: parent.width
            height: 2
            color: "#181825"

            Row {
                x: 1; y: 0
                spacing: 2

                Text {
                    text: "⚡ DUAL TERMINAL WORKFLOW & DATA INTERCHANGE"
                    color: "#89dceb"
                    bold: true
                }

                Text {
                    text: "│ [F1] T1  [F2] T2  [F3] Toggle  [Esc/F4] Unfocus  [Ctrl+Q] Quit"
                    color: "#f9e2af"
                }

                Text {
                    text: "│ Target: " + (commandTarget === 0 ? "Terminal 1" : (commandTarget === 1 ? "Terminal 2" : "BROADCAST (BOTH)"))
                    color: commandTarget === 2 ? "#cba6f7" : (commandTarget === 0 ? "#89dceb" : "#a6e3a1")
                    bold: true
                }
            }
        }

        // =============================================================
        // 2. MAIN DUAL TERMINALS SPLIT VIEW
        // =============================================================
        Row {
            width: parent.width
            height: Math.max(12, parent.height - 14)
            spacing: 0

            // ---------------------------------------------------------
            // LEFT PANEL: TERMINAL 1
            // ---------------------------------------------------------
            Rectangle {
                width: parent.width / 2
                height: parent.height
                border.style: "single"
                border.color: terminal1.activeFocus ? "#89dceb" : "#45475a"
                color: "#11111b"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    // Header bar
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: terminal1.activeFocus ? "#1e2030" : "#181825"

                        Row {
                            x: 1; y: 0
                            spacing: 1

                            Text {
                                text: " [T1] " + (terminal1.title ? terminal1.title : "Shell")
                                color: "#89dceb"
                                bold: true
                            }

                            Text {
                                text: "PID: " + terminal1.processId
                                color: "#cdd6f4"
                            }

                            Text {
                                text: terminal1.running ? "[RUNNING]" : "[STOPPED]"
                                color: terminal1.running ? "#a6e3a1" : "#f38ba8"
                            }

                            Text {
                                text: terminal1.activeFocus ? "◄ ACTIVE FOCUS ►" : ""
                                color: "#f9e2af"
                                bold: true
                            }
                        }
                    }

                    // Terminal 1 View Component
                    Rectangle {
                        width: parent.width
                        height: parent.height - 1
                        color: "black"

                        TerminalView {
                            id: terminal1
                            anchors.fill: parent
                            focus: true
                            program: "/bin/sh"
                            arguments: ["-l"]
                            autoStart: true

                            onFinished: (code) => {
                                logMessage = "Terminal 1 exited with code " + code
                            }

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier)) {
                                    root.quitApp()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_F4) {
                                    cmdInput.focus = true
                                    root.logMessage = "Unfocused Terminal 1 -> Focused Command Input [Esc/F4]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F1) {
                                    terminal1.focus = true
                                    root.commandTarget = 0
                                    root.logMessage = "Focus switched to Terminal 1 [F1]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F2) {
                                    terminal2.focus = true
                                    root.commandTarget = 1
                                    root.logMessage = "Focus switched to Terminal 2 [F2]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F3) {
                                    root.toggleTerminalFocus()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Tab) {
                                    if (event.modifiers & Qt.ShiftModifier) {
                                        cmdInput.focus = true
                                    } else {
                                        terminal2.focus = true
                                        root.commandTarget = 1
                                    }
                                    root.logMessage = "Focus cycled [Tab]"
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------
            // RIGHT PANEL: TERMINAL 2
            // ---------------------------------------------------------
            Rectangle {
                width: parent.width / 2
                height: parent.height
                border.style: "single"
                border.color: terminal2.activeFocus ? "#a6e3a1" : "#45475a"
                color: "#11111b"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    // Header bar
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: terminal2.activeFocus ? "#1e2030" : "#181825"

                        Row {
                            x: 1; y: 0
                            spacing: 1

                            Text {
                                text: " [T2] " + (terminal2.title ? terminal2.title : "Shell")
                                color: "#a6e3a1"
                                bold: true
                            }

                            Text {
                                text: "PID: " + terminal2.processId
                                color: "#cdd6f4"
                            }

                            Text {
                                text: terminal2.running ? "[RUNNING]" : "[STOPPED]"
                                color: terminal2.running ? "#a6e3a1" : "#f38ba8"
                            }

                            Text {
                                text: terminal2.activeFocus ? "◄ ACTIVE FOCUS ►" : ""
                                color: "#f9e2af"
                                bold: true
                            }
                        }
                    }

                    // Terminal 2 View Component
                    Rectangle {
                        width: parent.width
                        height: parent.height - 1
                        color: "black"

                        TerminalView {
                            id: terminal2
                            anchors.fill: parent
                            program: "/bin/sh"
                            arguments: ["-l"]
                            autoStart: true

                            onFinished: (code) => {
                                logMessage = "Terminal 2 exited with code " + code
                            }

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier)) {
                                    root.quitApp()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_F4) {
                                    cmdInput.focus = true
                                    root.logMessage = "Unfocused Terminal 2 -> Focused Command Input [Esc/F4]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F1) {
                                    terminal1.focus = true
                                    root.commandTarget = 0
                                    root.logMessage = "Focus switched to Terminal 1 [F1]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F2) {
                                    terminal2.focus = true
                                    root.commandTarget = 1
                                    root.logMessage = "Focus switched to Terminal 2 [F2]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F3) {
                                    root.toggleTerminalFocus()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Tab) {
                                    if (event.modifiers & Qt.ShiftModifier) {
                                        terminal1.focus = true
                                        root.commandTarget = 0
                                    } else {
                                        cmdInput.focus = true
                                    }
                                    root.logMessage = "Focus cycled [Tab]"
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }
            }
        }

        // =============================================================
        // 3. INTERCHANGE DATA & CONTROL SECTION (Height: 12)
        // =============================================================
        Rectangle {
            width: parent.width
            height: 12
            border.style: "single"
            border.color: "#585b70"
            color: "#181825"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                // Row A: Section Title
                Text {
                    text: "── INTER-TERMINAL DATA INTERCHANGE ACTIONS ───────────────────────────────────────────"
                    color: "#f9e2af"
                    bold: true
                }

                // Row B: Quick Transfer Action Buttons (Height: 3)
                Row {
                    spacing: 1

                    Button {
                        label: "[ Focus T1 ]"
                        normalBorderColor: terminal1.activeFocus ? "#89dceb" : "gray"
                        onClicked: {
                            terminal1.focus = true
                            commandTarget = 0
                            logMessage = "Focus set to Terminal 1"
                        }
                    }

                    Button {
                        label: "[ Focus T2 ]"
                        normalBorderColor: terminal2.activeFocus ? "#a6e3a1" : "gray"
                        onClicked: {
                            terminal2.focus = true
                            commandTarget = 1
                            logMessage = "Focus set to Terminal 2"
                        }
                    }

                    Button {
                        label: "[ T1 ➔ T2 ]"
                        normalBorderColor: "#89dceb"
                        onClicked: {
                            terminal2.sendText("echo 'Data payload from Terminal 1 -> Terminal 2'\n")
                            logMessage = "[T1 ➔ T2] Transferred message payload to Terminal 2"
                        }
                    }

                    Button {
                        label: "[ T2 ➔ T1 ]"
                        normalBorderColor: "#a6e3a1"
                        onClicked: {
                            terminal1.sendText("echo 'Data payload from Terminal 2 -> Terminal 1'\n")
                            logMessage = "[T2 ➔ T1] Transferred message payload to Terminal 1"
                        }
                    }

                    Button {
                        label: "[ Pipe T1 Env -> T2 ]"
                        normalBorderColor: "#cba6f7"
                        onClicked: {
                            terminal1.sendText("echo '=== T1 Environment ==='; env | head -n 3\n")
                            terminal2.sendText("echo 'Received env trigger from T1'\n")
                            logMessage = "Piped environment query in T1 & notification in T2"
                        }
                    }

                    Button {
                        label: "[ Sync PWD ]"
                        normalBorderColor: "#f9e2af"
                        onClicked: {
                            terminal1.sendText("pwd\n")
                            terminal2.sendText("pwd\n")
                            logMessage = "Dispatched directory check 'pwd' to both terminals"
                        }
                    }

                    Button {
                        label: "[ Clear Both ]"
                        normalBorderColor: "#f38ba8"
                        onClicked: {
                            terminal1.clearScrollback()
                            terminal2.clearScrollback()
                            logMessage = "Cleared scrollback for both terminals"
                        }
                    }
                }

                // Row C: Interactive Command Dispatcher (Height: 3)
                Row {
                    spacing: 1

                    Text {
                        text: "Dispatch Cmd: "
                        color: cmdInput.activeFocus ? "#f9e2af" : "#cdd6f4"
                        bold: cmdInput.activeFocus
                    }

                    Rectangle {
                        width: 38
                        height: 3
                        color: cmdInput.activeFocus ? "#1e1e2e" : "#11111b"
                        border.style: cmdInput.activeFocus ? "double" : "single"
                        border.color: cmdInput.activeFocus ? "#f9e2af" : "#45475a"

                        TextInput {
                            id: cmdInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "echo 'Hello Qtui Dual Terminal!'"
                            color: activeFocus ? "#ffffff" : "#cdd6f4"
                            focusable: true

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier)) {
                                    root.quitApp()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape) {
                                    root.focus = true
                                    root.logMessage = "Focus moved to Root Window [Esc]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F1) {
                                    terminal1.focus = true
                                    root.commandTarget = 0
                                    root.logMessage = "Focus switched to Terminal 1 [F1]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F2) {
                                    terminal2.focus = true
                                    root.commandTarget = 1
                                    root.logMessage = "Focus switched to Terminal 2 [F2]"
                                    event.accepted = true
                                } else if (event.key === Qt.Key_F3) {
                                    root.toggleTerminalFocus()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Tab) {
                                    if (event.modifiers & Qt.ShiftModifier) {
                                        terminal2.focus = true
                                        root.commandTarget = 1
                                    } else {
                                        terminal1.focus = true
                                        root.commandTarget = 0
                                    }
                                    root.logMessage = "Focus cycled [Tab]"
                                    event.accepted = true
                                }
                            }

                            onAccepted: {
                                dispatchCommand()
                            }
                        }
                    }

                    Button {
                        label: commandTarget === 0 ? "[ Target: T1 (Active) ]" : "[ Target: T1 ]"
                        normalBorderColor: commandTarget === 0 ? "#89dceb" : "gray"
                        onClicked: {
                            commandTarget = 0
                            logMessage = "Dispatcher target set to Terminal 1"
                        }
                    }

                    Button {
                        label: commandTarget === 1 ? "[ Target: T2 (Active) ]" : "[ Target: T2 ]"
                        normalBorderColor: commandTarget === 1 ? "#a6e3a1" : "gray"
                        onClicked: {
                            commandTarget = 1
                            logMessage = "Dispatcher target set to Terminal 2"
                        }
                    }

                    Button {
                        label: commandTarget === 2 ? "[ Target: BOTH (Broadcast) ]" : "[ Target: BOTH ]"
                        normalBorderColor: commandTarget === 2 ? "#cba6f7" : "gray"
                        onClicked: {
                            commandTarget = 2
                            logMessage = "Dispatcher target set to Broadcast (Both terminals)"
                        }
                    }

                    Button {
                        label: "[ Dispatch ➔ ]"
                        normalBorderColor: "#f9e2af"
                        onClicked: {
                            dispatchCommand()
                        }
                    }
                }

                // Row D: Audit Log Bar (Height: 1)
                Row {
                    spacing: 1

                    Text {
                        text: "Audit Trail: "
                        color: "#a6adc8"
                        bold: true
                    }

                    Text {
                        text: logMessage
                        color: "#89dceb"
                    }
                }
            }
        }
    }

    function dispatchCommand() {
        var cmd = cmdInput.text.trim()
        if (cmd.length === 0) return

        var payload = cmd + "\n"
        if (commandTarget === 0) {
            terminal1.sendText(payload)
            logMessage = "Dispatched command to Terminal 1: '" + cmd + "'"
        } else if (commandTarget === 1) {
            terminal2.sendText(payload)
            logMessage = "Dispatched command to Terminal 2: '" + cmd + "'"
        } else if (commandTarget === 2) {
            terminal1.sendText(payload)
            terminal2.sendText(payload)
            logMessage = "BROADCAST command to T1 & T2: '" + cmd + "'"
        }
    }
}



