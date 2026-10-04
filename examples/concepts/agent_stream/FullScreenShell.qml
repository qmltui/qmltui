import QtQuick
import Qtui
import Qtui.Controls
import Qtui.TerminalHost

Rectangle {
    id: fullScreenShell

    Theme { id: theme }

    property alias shellProgram: shellTerm.program
    signal returnToStudioRequested()

    color: "#090d16"

    function focusShell() {
        if (!shellTerm.running) {
            shellTerm.start()
        }
        shellTerm.focus = true
    }

    // Sleek Full-Width Top Header Bar (1 line)
    Row {
        id: headerRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        spacing: 1

        StatusBadge {
            text: "SHELL"
            status: "info"
            variant: "subtle"
            showIcon: false
        }

        Text {
            text: shellTerm.program
            color: theme.cyan
            bold: true
        }

        Text {
            text: "· PID: " + (shellTerm.processId > 0 ? shellTerm.processId : "active")
            color: theme.slate
        }

        Text {
            text: "· ~/Qtui · [Shift+PgUp/PgDn] Scroll · [Ctrl+T / Esc] Return"
            color: theme.amber
        }
    }

    // Full Viewport TerminalView with Native Scrollbar and Scrollback Buffer
    TerminalView {
        id: shellTerm
        anchors.top: headerRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        arguments: ["-l"]
        autoStart: true
        showScrollbar: true
        scrollbackLimit: 5000

        foregroundColor: theme.white
        backgroundColor: "#090d16"

        palette: [
            "#1e293b", // 0: Black (slate-800)
            theme.rose,  // 1: Red (#f87171)
            theme.green, // 2: Green (#34d399)
            theme.amber, // 3: Yellow (#fbbf24)
            theme.cyan,  // 4: Blue/Cyan (#38bdf8)
            theme.violet,// 5: Magenta (#a78bfa)
            "#22d3ee", // 6: Cyan (#22d3ee)
            theme.slate, // 7: White/Light Slate (#94a3b8)
            "#475569", // 8: Bright Black (#475569)
            "#fca5a5", // 9: Bright Red (#fca5a5)
            "#6ee7b7", // 10: Bright Green (#6ee7b7)
            "#fde047", // 11: Bright Yellow (#fde047)
            "#7dd3fc", // 12: Bright Blue (#7dd3fc)
            "#c4b5fd", // 13: Bright Magenta (#c4b5fd)
            "#67e8f9", // 14: Bright Cyan (#67e8f9)
            theme.white  // 15: Bright White (#f8fafc)
        ]

        onFinished: (code) => {
            fullScreenShell.returnToStudioRequested()
            // Auto-restart shell session so it is always persistent and ready
            shellTerm.start()
        }

        Keys.priority: Keys.BeforeItem
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape || ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_T || event.key === Qt.Key_B))) {
                fullScreenShell.returnToStudioRequested()
                event.accepted = true
            } else if (event.key === Qt.Key_F1) {
                fullScreenShell.returnToStudioRequested()
                event.accepted = true
            } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Q) {
                Qt.quit()
                event.accepted = true
            }
        }
    }
}
