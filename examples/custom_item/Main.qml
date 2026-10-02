import QtQml 2.15
import Tui 1.0
import CustomItemDemo 1.0

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    width: 80
    height: 24
    backgroundColor: "black"
    useAlternateScreen: true

    property Timer timer: Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: {
            // Toggle between 0.0 and 1.0. With Behavior, this should transition smoothly!
            progBar.value = (progBar.value === 0.0) ? 1.0 : 0.0;
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2
        width: 44

        Text {
            width: parent.width
            text: "Animation Driver Test"
            color: "cyan"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
        }

        ProgressBar {
            id: progBar
            width: parent.width
            height: 3
            border.style: "single"
            border.color: "gray"
            barColor: "green"
            value: 0.0

            Behavior on value {
                NumberAnimation {
                    duration: 1000
                }
            }
        }
    }
}
