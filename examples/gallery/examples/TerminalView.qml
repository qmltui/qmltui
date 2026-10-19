pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import QmlTui.TerminalHost

Item {
    id: root

    property int commandTarget: 2 // 0: T1, 1: T2, 2: Both
    property string logMessage: "Ready. Press F1/F2 to focus a shell, F3 to toggle, F4 for command input."

    readonly property string activeFocusLabel: {
        if (terminal1.terminalFocused)
            return "Terminal 1 (Left)";
        if (terminal2.terminalFocused)
            return "Terminal 2 (Right)";
        if (cmdInput.activeFocus)
            return "Command Dispatcher";
        return "Stage";
    }

    function hostTerminal(): Terminal {
        let p = parent;
        while (p && p.capabilities === undefined)
            p = p.parent;
        return p as Terminal;
    }

    function focusShell(index: int) {
        if (index === 0) {
            terminal1.focusTerminal();
            root.commandTarget = 0;
            root.logMessage = "Focus switched to Terminal 1 [F1]";
        } else {
            terminal2.focusTerminal();
            root.commandTarget = 1;
            root.logMessage = "Focus switched to Terminal 2 [F2]";
        }
    }

    function toggleTerminalFocus() {
        focusShell(terminal1.terminalFocused ? 1 : 0);
    }

    function focusCommandInput() {
        cmdInput.focus = true;
        root.logMessage = "Focused Command Input [F4]";
    }

    function handleHotkey(event: KeyEvent): bool {
        if (event.key === Qt.Key_F1) {
            root.focusShell(0);
            return true;
        }
        if (event.key === Qt.Key_F2) {
            root.focusShell(1);
            return true;
        }
        if (event.key === Qt.Key_F3) {
            root.toggleTerminalFocus();
            return true;
        }
        if (event.key === Qt.Key_F4) {
            root.focusCommandInput();
            return true;
        }
        return false;
    }

    function dispatchCommand() {
        const cmd = cmdInput.text.trim();
        if (cmd.length === 0)
            return;
        const payload = cmd + "\n";
        if (root.commandTarget === 0 || root.commandTarget === 2)
            terminal1.sendText(payload);
        if (root.commandTarget === 1 || root.commandTarget === 2)
            terminal2.sendText(payload);
        root.logMessage = "Dispatched: " + cmd;
    }

    Component.onDestruction: {
        terminal1.kill();
        terminal2.kill();
    }

    Keys.onPressed: event => {
        if (root.handleHotkey(event))
            event.accepted = true;
    }

    component ActionButton: Button {
        property string accentColor: "#8b949e"
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0
        textColor: activeFocus ? "#00ff66" : accentColor
    }

    component ShellPane: Rectangle {
        id: pane

        property string tag: "T1"
        property string accentColor: "#89dceb"
        readonly property bool terminalFocused: termView.activeFocus
        readonly property bool running: termView.running

        signal hotkeyTriggered(KeyEvent event)
        signal escapeRequested
        signal exited(int code)

        function focusTerminal() {
            termView.focus = true;
        }

        function sendText(payload: string) {
            termView.sendText(payload);
        }

        function clearScrollback() {
            termView.clearScrollback();
        }

        function kill() {
            termView.kill();
        }

        color: "#0d1117"
        border {
            style: pane.terminalFocused ? Border.Double : Border.Single
            color: pane.terminalFocused ? pane.accentColor : "#30363d"
        }

        Column {
            anchors {
                fill: parent
                margins: 1
            }
            spacing: 0

            Rectangle {
                width: parent.width
                height: 1
                clip: true
                color: pane.terminalFocused ? "#161b22" : "#0d1117"

                Row {
                    x: 1
                    spacing: 1

                    Text {
                        text: "[" + pane.tag + "] " + (termView.title ? termView.title : "Shell")
                        color: pane.accentColor
                        bold: true
                    }

                    Text {
                        text: "PID:" + termView.processId
                        color: "#c9d1d9"
                    }

                    Text {
                        text: termView.running ? "[RUNNING]" : "[STOPPED]"
                        color: termView.running ? "#a6e3a1" : "#f87171"
                    }

                    Text {
                        visible: pane.terminalFocused
                        text: "◄ FOCUS"
                        color: "#fbbf24"
                        bold: true
                    }
                }
            }

            TerminalView {
                id: termView

                width: parent.width
                height: Math.max(1, parent.height - 1)
                program: "/bin/sh"
                arguments: ["-l"]
                autoStart: true

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Escape) {
                        pane.escapeRequested();
                        event.accepted = true;
                    } else {
                        pane.hotkeyTriggered(event);
                    }
                }

                onFinished: code => {
                    pane.exited(code);
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 2
            color: "#161b22"

            Row {
                x: 1
                y: 0
                spacing: 2

                Text {
                    text: "DUAL TERMINAL WORKFLOW"
                    color: "#a6e3a1"
                    bold: true
                }

                Text {
                    text: "Focus: " + root.activeFocusLabel
                    color: "#38bdf8"
                }
            }

            Text {
                x: 1
                y: 1
                text: "F1/F2: focus shell · F3: toggle · F4: command input · Esc: back"
                color: "#8b949e"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            ShellPane {
                id: terminal1

                Layout.fillWidth: true
                Layout.fillHeight: true
                tag: "T1"
                accentColor: "#38bdf8"
                onHotkeyTriggered: event => {
                    if (root.handleHotkey(event))
                        event.accepted = true;
                }
                onEscapeRequested: {
                    const host = root.hostTerminal();
                    if (host && typeof host.closeDemo === "function")
                        host.closeDemo();
                    else
                        Qt.quit();
                }
                onExited: code => {
                    root.logMessage = "Terminal 1 exited (" + code + ")";
                }
            }

            ShellPane {
                id: terminal2

                Layout.fillWidth: true
                Layout.fillHeight: true
                tag: "T2"
                accentColor: "#a6e3a1"
                onHotkeyTriggered: event => {
                    if (root.handleHotkey(event))
                        event.accepted = true;
                }
                onEscapeRequested: {
                    const host = root.hostTerminal();
                    if (host && typeof host.closeDemo === "function")
                        host.closeDemo();
                    else
                        Qt.quit();
                }
                onExited: code => {
                    root.logMessage = "Terminal 2 exited (" + code + ")";
                }
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.preferredHeight: 5
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Command Dispatcher & Inter-Terminal Actions"
                color: "#fbbf24"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Row {
                    spacing: 1

                    ActionButton {
                        text: "[F1: T1]"
                        accentColor: "#38bdf8"
                        onClicked: root.focusShell(0)
                    }

                    ActionButton {
                        text: "[F2: T2]"
                        accentColor: "#a6e3a1"
                        onClicked: root.focusShell(1)
                    }

                    ActionButton {
                        text: "[F3: Swap]"
                        accentColor: "#fbbf24"
                        onClicked: root.toggleTerminalFocus()
                    }

                    ActionButton {
                        text: root.commandTarget === 0 ? "[Target: T1]" : (root.commandTarget === 1 ? "[Target: T2]" : "[Target: Both]")
                        accentColor: "#c084fc"
                        onClicked: {
                            root.commandTarget = (root.commandTarget + 1) % 3;
                        }
                    }

                    ActionButton {
                        text: "[Clear]"
                        accentColor: "#f87171"
                        onClicked: {
                            terminal1.clearScrollback();
                            terminal2.clearScrollback();
                            root.logMessage = "Cleared scrollback";
                        }
                    }
                }

                Row {
                    spacing: 1

                    Text {
                        text: "Cmd:"
                        color: cmdInput.activeFocus ? "#fbbf24" : "#c9d1d9"
                        bold: cmdInput.activeFocus
                    }

                    Rectangle {
                        width: Math.max(24, root.width - 20)
                        height: 1
                        color: cmdInput.activeFocus ? "#161b22" : "#0d1117"

                        TextInput {
                            id: cmdInput

                            anchors.fill: parent
                            focus: true
                            text: "echo 'Hello QmlTui Dual Terminal!'"
                            color: "#f8fafc"

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: event => {
                                if (root.handleHotkey(event))
                                    event.accepted = true;
                            }

                            onAccepted: root.dispatchCommand()
                        }
                    }

                    ActionButton {
                        text: "[Dispatch]"
                        accentColor: "#2cde85"
                        onClicked: root.dispatchCommand()
                    }
                }

                Text {
                    text: "Audit: " + root.logMessage
                    color: "#8b949e"
                }
            }
        }
    }
}
