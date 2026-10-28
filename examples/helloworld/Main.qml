pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    property int count: 0

    title: count === 0 ? "Hello QmlTui!" : "Interactions: " + root.count
    color: "#0d1117"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            console.log("Goodbye!");
            Qt.quit();
        } else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            root.count++;
        } else if (event.key === Qt.Key_R) {
            root.count = 0;
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 52
        height: 11
        border {
            style: Border.Single
            color: "#2cde85"
        }

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                text: "Hello, QmlTui!"
                color: "#00ff66"
                bold: true
            }

            Text {
                text: "QML Declarative terminal UIs"
                color: "#2cde85"
                dim: true
            }

            Text {
                text: "Interactions: " + root.count
                color: "#c9d1d9"
            }

            Text {
                text: "[Enter] Count  •  [R] Reset  •  [Q] Quit"
                color: "#8b949e"
                dim: true
            }
        }
    }
}
