import QtQml
import Qtui

Terminal {
    id: root
    color: "#0d1117"

    property string message: qsTr("↑/↓ navigate • Enter select • Q quit")

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            text: qsTr("⚡ SERVICE MONITOR")
            color: "cyan"
            bold: true
        }

        TableView {
            focus: true
            width: parent.width
            height: parent.height - 4
            alternatingRows: true
            showGrid: true

            columns: [
                { title: qsTr("SERVICE"), width: 24 },
                { title: qsTr("LATENCY"), width: 12, align: "right" },
                { title: qsTr("STATUS"), width: 16, align: "center" }
            ]

            model: [
                ["api-gateway", "12 ms", "healthy"],
                ["postgres", "8 ms", "healthy"],
                ["worker-01", "86 ms", "degraded"]
            ]

            cellFormat: (row, column, value) => {
                if (column !== 2)
                    return { text: value }

                const healthy = value === "healthy"
                return {
                    text: healthy ? qsTr("● HEALTHY") : qsTr("▲ DEGRADED"),
                    color: healthy ? "green" : "yellow",
                    bold: true
                }
            }

            onRowActivated: (row) => {
                root.message = qsTr("Selected service row %1").arg(row + 1)
            }
        }

        Text {
            text: root.message
            color: "gray"
        }
    }
}
