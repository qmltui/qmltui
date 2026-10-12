pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {

        id: rootWindow
        width: 80
        height: 24
        color: "black"

        property Timer timer: Timer {
            interval: 2000
            running: true
            repeat: true
            onTriggered: {
                // Toggle between empty and full without optional Behavior support.
                progBar.value = (progBar.value === 0.0) ? 1.0 : 0.0;
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 2
            width: 44

            Text {
                width: parent.width
                text: "Custom C++ Item"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
            }

            ProgressBar {
                id: progBar
                width: parent.width
                height: 3
                border.style: Border.Single
                border.color: "gray"
                barColor: "green"
                value: 0.0

            }
        }
    }
}
