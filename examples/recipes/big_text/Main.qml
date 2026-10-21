pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        anchors {
            fill: parent
            margins: 1
        }
        spacing: 1

        Text {
            text: "BigText — 3x5 Block Glyph Item"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Custom C++ Item painting 3x5 block glyphs into TextGridBuffer. Press Q or Esc to quit."
            dim: true
        }

        Panel {
            width: Math.min(parent.width, 54)
            height: 7
            border {
                style: Border.Single
                color: "gray"
            }
            title: Text {
                text: "Banner"
                bold: true
            }

            BigText {
                anchors.centerIn: parent
                text: "QMLTUI 2026"
                color: "cyan"
            }
        }

        Panel {
            width: Math.min(parent.width, 54)
            height: 7
            border {
                style: Border.Single
                color: "gray"
            }
            title: Text {
                text: "Clock"
                bold: true
            }

            BigText {
                id: clock

                property string currentTime: Qt.formatDateTime(new Date(), "hh:mm:ss")

                anchors.centerIn: parent
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
