pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "🅰 BIGTEXT BANNER DEMO"
            color: "yellow"
            bold: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Press Q or ESC to quit"
            color: "gray"
        }

        Rectangle {
            width: parent.width
            height: 14
            border {
                style: Border.Double
                color: "yellow"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "HEADER BANNER:"
                    color: "cyan"
                }

                BigText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "QMLTUI 2026"
                    color: "cyan"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "DIGITAL CLOCK:"
                    color: "green"
                }

                BigText {
                    id: clock

                    property string currentTime: Qt.formatDateTime(new Date(), "hh:mm:ss")

                    anchors.horizontalCenter: parent.horizontalCenter
                    text: currentTime
                    color: "green"

                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        onTriggered: clock.currentTime = Qt.formatDateTime(new Date(), "hh:mm:ss")
                    }
                }
            }
        }
    }
}
