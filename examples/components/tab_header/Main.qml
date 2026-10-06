pragma ComponentBehavior: Bound
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

    Rectangle {
        width: 70
        height: 14


        Column {
            anchors.fill: parent
            spacing: 1

            Text {
                text: "🔖 TABHEADER WIDGET DEMO"
                color: "cyan"
                bold: true
            }

            Text {
                text: "Use LEFT/RIGHT arrow keys or click tabs to switch, Q/ESC to quit"
                color: "gray"
            }

            TabHeader {
                id: header
                focus: true
                width: 68
                activeTabColor: "white"
                activeTabBgColor: "blue"
                inactiveTabColor: "gray"
                tabs: ["Overview", "Telemetry", "Logs", "Settings"]
            }

            Text { text: ""; height: 1 }

            Text {
                text: "Active View: " + header.tabs[header.currentIndex]
                color: "yellow"
                bold: true
            }
        }
    }
}
