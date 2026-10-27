pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import QmlTui.PtyView

// PtyView: Embeds a child pseudo-terminal process inside a QmlTui layout.
// Available on Linux and macOS when QMLTUI_ENABLE_PTYVIEW=ON.
ColumnLayout {
    id: ptyDemo

    property string statusText: "Ready"

    function hostTerminal(): Terminal {
        let p = parent;
        while (p && p.capabilities === undefined)
            p = p.parent;
        return p as Terminal;
    }

    function sendCommand() {
        const cmd = cmdInput.text.trim();
        if (cmd.length === 0)
            return;
        shellView.sendText(cmd + "\n");
        ptyDemo.statusText = "Sent: " + cmd;
    }

    anchors.fill: parent
    spacing: 1

    Component.onDestruction: shellView.kill()

    // Status row showing process state and PID
    Row {
        spacing: 2

        Text {
            text: shellView.running ? "[RUNNING]" : "[STOPPED]"
            color: shellView.running ? "#a6e3a1" : "#f87171"
            bold: true
        }

        Text {
            text: "PID: " + shellView.processId + "  ·  " + ptyDemo.statusText
            color: "#8b949e"
        }
    }

    // Embedded PTY viewport running /bin/sh
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#161b22"

        PtyView {
            id: shellView
            anchors {
                fill: parent
                margins: 1
            }
            program: defaultUserShell()
            arguments: ["-l"]
            autoStart: true

            // Intercept Escape so the user can return to the Gallery grid
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Escape) {
                    const host = ptyDemo.hostTerminal();
                    if (host && typeof host.closeDemo === "function")
                        host.closeDemo();
                    else
                        Qt.quit();
                    event.accepted = true;
                }
            }
        }
    }

    // Command input bar dispatching text into the PTY via sendText()
    RowLayout {
        Layout.fillWidth: true
        spacing: 1

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            TextInput {
                id: cmdInput
                anchors.fill: parent
                focus: true
                text: "echo 'Hello from PtyView!'"
                color: "#f8fafc"
                onAccepted: ptyDemo.sendCommand()
            }
        }

        Button {
            text: "Send"
            onClicked: ptyDemo.sendCommand()
        }

        Button {
            text: "Clear"
            onClicked: shellView.clearScrollback()
        }
    }
}
