pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        id: rootWindow
        width: 80
        height: 24

        // Left Panel (Rounded with badge and footer)
        Panel {
            x: 2
            y: 2
            width: 32
            height: 10
            title: "System"
            titleBadge: "[ACTIVE]"
            titleBadgeColor: "green"
            titleColor: "cyan"
            border.color: "blue"
            border.style: "rounded"
            footer: "[h] Help"
            footerColor: "gray"

            Text {
                anchors.centerIn: parent
                text: "Content Area 1"
                color: "default"
            }
        }

        // Center Panel (Single with centered title, trailing metric, and footers)
        Panel {
            x: 36
            y: 2
            width: 32
            height: 10
            title: "Metrics"
            titleAlignment: Qt.AlignHCenter
            titleTrailing: "60 FPS"
            titleTrailingColor: "magenta"
            titleColor: "yellow"
            border.color: "green"
            border.style: "single"
            footer: "Load: 12%"
            footerTrailing: "RAM: 45%"

            Text {
                anchors.centerIn: parent
                text: "Content Area 2"
                color: "default"
            }
        }

        // Bottom Panel (Full-width rounded with title, badge, trailing header, and dual footers)
        Panel {
            x: 2
            y: 13
            width: 66
            height: 10
            title: "Diagnostic Log"
            titleAlignment: Qt.AlignLeft
            titleBadge: "[6 active]"
            titleBadgeColor: "yellow"
            titleTrailing: "v1.0.0"
            titleTrailingColor: "gray"
            titleColor: "magenta"
            border.color: "red"
            border.style: "rounded"
            footer: "[q] Quit"
            footerTrailing: "Status: OK"
            footerTrailingColor: "green"

            Text {
                anchors.centerIn: parent
                text: "Content Area 3"
                color: "default"
            }
        }
    }
}
