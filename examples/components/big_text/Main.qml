import Qtui.Extras
import QtQml
import Qtui


Terminal {
 

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "🅰 BIGTEXT BANNER DEMO"
            color: "yellow"
            bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Press Q or ESC to quit"
            color: "gray"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            width: parent.width
            height: 14
            border.style: "double"
            border.color: "yellow"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    text: "HEADER BANNER:"
                    color: "cyan"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                BigText {
                    text: "QTUI 2026"
                    color: "cyan"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "DIGITAL CLOCK:"
                    color: "green"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                BigText {
                    id: clock
                    text: Qt.formatDateTime(new Date(), "hh:mm:ss")
                    color: "green"
                    anchors.horizontalCenter: parent.horizontalCenter

                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        onTriggered: clock.text = Qt.formatDateTime(new Date(), "hh:mm:ss")
                    }
                }
            }
        }
    }
}
