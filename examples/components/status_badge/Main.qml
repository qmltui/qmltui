import QtQml 2.15
import Qtui
import Qtui.Controls

Terminal {
    id: root
    width: 80
    height: 28

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Qtui StatusBadge Component Demo"
            color: "cyan"
            bold: true
        }

        Text { text: "──────────────────────────────────────────────────────────"; color: "gray" }

        Text { text: "Standard Status States (Subtle Variant):"; color: "white"; bold: true }
        Row {
            spacing: 2
            StatusBadge { status: "success" }
            StatusBadge { status: "warning" }
            StatusBadge { status: "error" }
            StatusBadge { status: "info" }
        }

        Text { text: "Custom Text Labels & Custom Icons:"; color: "white"; bold: true }
        Row {
            spacing: 2
            StatusBadge { status: "success"; text: "ONLINE"; icon: "●" }
            StatusBadge { status: "warning"; text: "HIGH LOAD"; icon: "▲" }
            StatusBadge { status: "error"; text: "CRITICAL"; icon: "✖" }
            StatusBadge { status: "info"; text: "PENDING"; icon: "⌛" }
        }

        Text { text: "Outlined Variant:"; color: "white"; bold: true }
        Row {
            spacing: 2
            StatusBadge { variant: "outlined"; status: "success"; text: "SYSTEM OK" }
            StatusBadge { variant: "outlined"; status: "warning"; text: "DISK 92%" }
            StatusBadge { variant: "outlined"; status: "error"; text: "DB DOWN" }
        }

        Text { text: "Filled Variant (Inverted ANSI Colors):"; color: "white"; bold: true }
        Row {
            spacing: 2
            StatusBadge { variant: "filled"; status: "success"; text: " ACTIVE " }
            StatusBadge { variant: "filled"; status: "warning"; text: " PAUSED " }
            StatusBadge { variant: "filled"; status: "error"; text: " FAILED " }
            StatusBadge { variant: "filled"; status: "info"; text: " BUSY " }
        }

        Text { text: "──────────────────────────────────────────────────────────"; color: "gray" }

        Text {
            text: "Press 'Q' or 'Escape' to exit"
            color: "gray"
        }
    }
}
