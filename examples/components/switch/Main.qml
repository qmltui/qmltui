import QtQml 2.15
import Qtui
import Qtui.Controls

Terminal {
    id: root

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
            text: "Qtui Switch Component Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Switch {
            label: "Dark Theme Mode"
            checked: true
            onToggled: (val) => statusText.text = "Dark Theme: " + (val ? "ON" : "OFF")
        }

        Switch {
            label: "Vim Keybindings"
            checked: false
            onToggled: (val) => statusText.text = "Vim Mode: " + (val ? "ON" : "OFF")
        }

        Switch {
            label: "Telemetry Reporting"
            checked: true
            onToggled: (val) => statusText.text = "Telemetry: " + (val ? "ON" : "OFF")
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text {
            id: statusText
            text: "Status: Ready."
            color: "cyan"
        }
    }
}
