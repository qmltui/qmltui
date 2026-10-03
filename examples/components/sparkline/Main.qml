import Qtui.Extras
import QtQml 2.15
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
            text: "📈 SPARKLINE TELEMETRY DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Press Q or ESC to quit"
            color: "gray"
        }

        Rectangle {
            width: parent.width
            height: 20
            border.style: "single"
            border.color: "gray"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text { text: "CPU Core 1 Usage"; color: "green" }
                Sparkline {
                    width: 54
                    height: 1
                    values: [5, 10, 25, 40, 60, 80, 95, 70, 45, 30, 20, 15, 35, 55, 75, 90, 100]
                    color: "green"
                }

                Text { text: "Memory Bandwidth (GB/s)"; color: "magenta" }
                Sparkline {
                    width: 54
                    height: 1
                    values: [90, 85, 80, 75, 70, 65, 60, 55, 65, 75, 85, 90, 95, 80, 70]
                    color: "magenta"
                }

                Text { text: "Network Packets In/Out"; color: "yellow" }
                Sparkline {
                    width: 54
                    height: 1
                    values: [0, 5, 2, 45, 88, 12, 4, 0, 78, 92, 10, 2, 5, 60, 40]
                    color: "yellow"
                }
            }
        }
    }
}
