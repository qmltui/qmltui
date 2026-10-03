import QtQml 2.15
import Qtui
import Qtui.Extras


Terminal {
    width: 70
    height: 14

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
            text: "🎛 GAUGE PROGRESS BAR DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Demonstrates custom labels, bar fills, colors, and border styles"
            color: "gray"
        }

        Gauge {
            width: 68
            label: "CPU LOAD  : "
            value: 78
            gaugeColor: "yellow"
            borderStyle: "single"
            borderColor: "cyan"
        }

        Gauge {
            width: 68
            label: "RAM USAGE : "
            value: 45
            gaugeColor: "green"
        }

        Gauge {
            width: 68
            label: "DISK I/O  : "
            value: 92
            gaugeColor: "red"
            filledChar: "█"
            unfilledChar: "-"
        }
    }
}
