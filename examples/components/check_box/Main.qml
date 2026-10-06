import QtQml
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
            text: "Qtui CheckBox Component Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        CheckBox {
            text: "Enable Logging"
            checked: true
            onToggled: (val) => statusText.text = "Logging: " + val
        }

        CheckBox {
            text: "Show Hidden Files"
            checked: false
            onToggled: (val) => statusText.text = "Show Hidden: " + val
        }

        CheckBox {
            text: "Wrap Lines"
            checked: true
            onToggled: (val) => statusText.text = "Wrap Lines: " + val
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text {
            id: statusText
            text: "Status: Ready."
            color: "green"
        }
    }
}
