import QtQml 2.15
import Tui 1.0

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    backgroundColor: "black"

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    property var frames: ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    property int frameIndex: 0

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.frames.length
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2

        Text {
            text: "Loading System Resources..."
            color: "cyan"
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1

            Text {
                text: root.frames[root.frameIndex]
                color: "yellow"
            }

            Text {
                text: "Please wait"
                color: "white"
            }
        }
        
        Text {
            text: "Press 'Q' to quit"
            color: "gray"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
