import QtQml
import Qamui
import Qamui.Controls

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
            text: "Qamui ToolTip Component Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Button {
            id: helpBtn
            text: "[ Hover or Focus for ToolTip ]"
            ToolTip.text: "This is an attached ToolTip message!"
            ToolTip.visible: true
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text {
            text: "Hover mouse over button to show tooltip"
            color: "yellow"
        }
    }
}
