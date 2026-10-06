pragma ComponentBehavior: Bound
import QtQml
import Qtui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            if (filterInput.text.length > 0)
                filterInput.text = ""
            else
                Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Q && !filterInput.focus) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        id: root
        width: 80
        height: 24

        ProcessProxy {
            id: processes
            filterText: filterInput.text
            onActivatedPid: (pid) => { statusText.text = qsTr("Activated PID %1").arg(pid) }
        }


        Text {
            x: 1; y: 0; width: 76
            text: qsTr("TABLEVIEW — EXTERNAL SORTING AND FILTERING")
            color: "cyan"
            bold: true
        }
        Text {
            x: 1; y: 1; width: 76
            text: qsTr("Click headers to sort; select a row to activate its source PID.")
        }
        Text { x: 1; y: 3; text: qsTr("Filter:") }
        TextInput {
            id: filterInput
            objectName: "filterInput"
            x: 9; y: 3; width: 30; height: 1
            placeholderText: qsTr("Process name")
        }
        Text { x: 1; y: 5; width: 76; text: processes.sortLabel; color: "yellow" }

        Rectangle {
            x: 1; y: 7; width: 76; height: 12
            border.style: "single"
            border.color: "cyan"
            TableView {
                id: table
                objectName: "table"
                focus: true
                anchors.fill: parent
                anchors.margins: 1
                alternatingRows: true
                showGrid: true
                columns: [
                    { title: "PID", width: 8 },
                    { title: "PROCESS NAME", width: 24 },
                    { title: "CPU%", width: 12, align: "right" },
                    { title: "STATUS", width: 16, align: "center" }
                ]
                model: processes
                onHeaderClicked: (column) => processes.headerClicked(column)
                onRowActivated: (row) => processes.activate(row)
                cellFormat: (row, column, value) => {
                    if (column === 3)
                        return { text: value, color: value === "RUNNING" ? "green" : "yellow" }
                    return { text: value }
                }
            }
        }
        Text {
            id: statusText
            x: 1; y: 20; width: 76
            text: qsTr("Select a process")
            color: "cyan"
        }
        Text {
            x: 1; y: 22; width: 76
            text: qsTr("Esc clears the filter, then quits. Q quits outside the filter.")
        }
    }
}
