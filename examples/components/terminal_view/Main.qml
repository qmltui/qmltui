pragma ComponentBehavior: Bound

import QtQml
import Qamui
import Qamui.Controls
import Qamui.TerminalHost

Terminal {
    id: root
    mode: Terminal.AltScreen
    title: "TerminalView — dual shell workflow"
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
                    text: "│ Target: " + (root.commandTarget === 0 ? "Terminal 1" : (root.commandTarget === 1 ? "Terminal 2" : "BROADCAST (BOTH)"))
                    color: root.commandTarget === 2 ? "#cba6f7" : (root.commandTarget === 0 ? "#89dceb" : "#a6e3a1")
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
                width: Math.floor(parent.width / 2)
                height: parent.height
                border.style: Border.Single
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
                                root.logMessage = "Terminal 1 exited with code " + code
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
                width: parent.width - Math.floor(parent.width / 2)
                height: parent.height
                border.style: Border.Single
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
                                root.logMessage = "Terminal 2 exited with code " + code
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
            border.style: Border.Single
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
                        background: Rectangle {
                            color: "default"
                            border.color: terminal1.activeFocus ? "#89dceb" : "gray"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Focus T1 ]"
                        onClicked: {
                            terminal1.focus = true
                            root.commandTarget = 0
                            root.logMessage = "Focus set to Terminal 1"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: terminal2.activeFocus ? "#a6e3a1" : "gray"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Focus T2 ]"
                        onClicked: {
                            terminal2.focus = true
                            root.commandTarget = 1
                            root.logMessage = "Focus set to Terminal 2"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#89dceb"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ T1 ➔ T2 ]"
                        onClicked: {
                            terminal2.sendText("echo 'Data payload from Terminal 1 -> Terminal 2'\n")
                            root.logMessage = "[T1 ➔ T2] Transferred message payload to Terminal 2"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#a6e3a1"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ T2 ➔ T1 ]"
                        onClicked: {
                            terminal1.sendText("echo 'Data payload from Terminal 2 -> Terminal 1'\n")
                            root.logMessage = "[T2 ➔ T1] Transferred message payload to Terminal 1"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#cba6f7"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Pipe T1 Env -> T2 ]"
                        onClicked: {
                            terminal1.sendText("echo '=== T1 Environment ==='; env | head -n 3\n")
                            terminal2.sendText("echo 'Received env trigger from T1'\n")
                            root.logMessage = "Piped environment query in T1 & notification in T2"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#f9e2af"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Sync PWD ]"
                        onClicked: {
                            terminal1.sendText("pwd\n")
                            terminal2.sendText("pwd\n")
                            root.logMessage = "Dispatched directory check 'pwd' to both terminals"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#f38ba8"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Clear Both ]"
                        onClicked: {
                            terminal1.clearScrollback()
                            terminal2.clearScrollback()
                            root.logMessage = "Cleared scrollback for both terminals"
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
                        border.style: cmdInput.activeFocus ? Border.Double : Border.Single
                        border.color: cmdInput.activeFocus ? "#f9e2af" : "#45475a"

                        TextInput {
                            id: cmdInput
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "echo 'Hello Qamui Dual Terminal!'"
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
                                root.dispatchCommand()
                            }
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: root.commandTarget === 0 ? "#89dceb" : "gray"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: root.commandTarget === 0 ? "[ Target: T1 (Active) ]" : "[ Target: T1 ]"
                        onClicked: {
                            root.commandTarget = 0
                            root.logMessage = "Dispatcher target set to Terminal 1"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: root.commandTarget === 1 ? "#a6e3a1" : "gray"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: root.commandTarget === 1 ? "[ Target: T2 (Active) ]" : "[ Target: T2 ]"
                        onClicked: {
                            root.commandTarget = 1
                            root.logMessage = "Dispatcher target set to Terminal 2"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: root.commandTarget === 2 ? "#cba6f7" : "gray"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: root.commandTarget === 2 ? "[ Target: BOTH (Broadcast) ]" : "[ Target: BOTH ]"
                        onClicked: {
                            root.commandTarget = 2
                            root.logMessage = "Dispatcher target set to Broadcast (Both terminals)"
                        }
                    }

                    Button {
                        background: Rectangle {
                            color: "default"
                            border.color: "#f9e2af"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
                        }
                        text: "[ Dispatch ➔ ]"
                        onClicked: {
                            root.dispatchCommand()
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
                        text: root.logMessage
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
