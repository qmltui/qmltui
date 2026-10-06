import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: rootWindow
    width: 80
    height: 24
    fillTerminal: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    // Left Panel
    Panel {
        x: 2
        y: 2
        width: 30
        height: 10
        title: "Left Aligned"
        titleAlignment: Qt.AlignLeft
        titleColor: "cyan"
        border.color: "blue"
        border.style: "double"

        Text {
            anchors.centerIn: parent
            text: "Content Area 1"
            color: "white"
        }
    }

    // Center Panel
    Panel {
        x: 35
        y: 2
        width: 30
        height: 10
        title: "Center Aligned"
        titleAlignment: Qt.AlignHCenter
        titleColor: "yellow"
        border.color: "green"
        border.style: "single"

        Text {
            anchors.centerIn: parent
            text: "Content Area 2"
            color: "white"
        }
    }

    // Right Panel
    Panel {
        x: 2
        y: 13
        width: 63
        height: 10
        title: "Right Aligned"
        titleAlignment: Qt.AlignRight
        titleColor: "magenta"
        border.color: "red"
        border.style: "double"

        Text {
            anchors.centerIn: parent
            text: "Content Area 3"
            color: "white"
        }
    }
}
