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
            text: "Qtui RadioButton Component Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text { text: "Select Build Configuration:"; color: "yellow" }

        ButtonGroup {
            buttons: [r1, r2, r3]
        }

        RadioButton {
            id: r1
            text: "Debug Build"
            checked: true
            onToggled: (val) => statusText.text = "Selected Debug Build"
        }

        RadioButton {
            id: r2
            text: "Release Build"
            onToggled: (val) => statusText.text = "Selected Release Build"
        }

        RadioButton {
            id: r3
            text: "MinSizeRel Build"
            onToggled: (val) => statusText.text = "Selected MinSizeRel Build"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text {
            id: statusText
            text: "Status: Ready."
            color: "yellow"
        }
    }
}
