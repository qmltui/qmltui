import QtQml
import Qamui

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            text: "Qamui ScrollView Component Example (Use Up/Down/PageUp/PageDown/Wheel to scroll)"
            color: "cyan"
        }

        ScrollView {
            focus: true
            width: 45
            height: 12

            Column {
                spacing: 0
                Repeater {
                    model: 30
                    Text {
                        required property int index
                        text: "Scrollable Content Line #" + (index + 1)
                        color: (index % 2 === 0) ? "default" : "yellow"
                    }
                }
            }
        }
    }
}
