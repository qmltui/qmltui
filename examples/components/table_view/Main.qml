import QtQml
import Qtui


Terminal {
    id: root
    width: 75
    height: 20

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
            text: "📊 TABLEVIEW SORTING & ALIGNMENT DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Click column headers to sort! UP/DOWN to navigate, Q/ESC to quit"
            color: "gray"
        }

        // Table 1: Sorting and Column Alignment Demo
        Rectangle {
            width: 73
            height: 12
            border.style: "single"
            border.color: "cyan"

            TableView {
                focus: true
                anchors.fill: parent
                anchors.margins: 1
                headerColor: "yellow"
                headerBgColor: "#1e1e2e"
                selectionColor: "white"
                selectionBgColor: "blue"
                alternatingRows: true
                showGrid: true

                sortColumn: 2
                sortOrder: Qt.DescendingOrder

                columns: [
                    { title: "PID", width: 8, align: "left" },
                    { title: "PROCESS NAME", width: 22, align: "left" },
                    { title: "CPU%", width: 10, align: "right" },
                    { title: "STATUS", width: 14, align: "center" }
                ]

                model: [
                    ["1001", "systemd", "0.1%", "RUNNING"],
                    ["1042", "terminal_engine", "12.4%", "RUNNING"],
                    ["1089", "qml_renderer", "4.2%", "RUNNING"],
                    ["1120", "dbus-daemon", "0.0%", "STOPPED"],
                    ["1250", "net_stack", "1.8%", "RUNNING"],
                    ["1304", "posix_tty", "0.5%", "CRASHED"],
                    ["1410", "input_decoder", "0.2%", "RUNNING"],
                    ["1520", "kworker/0:1", "0.0%", "STOPPED"]
                ]

                // Custom JS Cell Formatter
                cellFormat: (row, col, value) => {
                    if (col === 3) {
                        if (value === "RUNNING") return { text: "✔ RUNNING", color: "green", bold: true }
                        if (value === "STOPPED") return { text: "⏸ STOPPED", color: "yellow" }
                        if (value === "CRASHED") return { text: "✖ CRASHED", color: "red", bold: true }
                    }
                    return { text: value }
                }

                onRowActivated: (idx) => {
                    statusText.text = "Activated PID row #" + idx
                }
            }
        }

        Text {
            id: statusText
            text: "Status: Click headers to sort or select a row and press ENTER"
            color: "cyan"
        }
    }
}
