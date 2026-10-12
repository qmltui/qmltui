pragma ComponentBehavior: Bound
import QtQml
import QmlTui

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

        Row {
            x: 1; y: 7
            spacing: 1

            TableView {
                id: table
                objectName: "table"
                focus: true
                height: 12
                showOuterBorder: true
                alternatingRows: true
                showColumnGrid: true
                gridStyle: TableView.Double
                columns: [
                    { title: "PID", width: 8, padding: 1 },
                    { title: "PROCESS NAME", width: 18, padding: 1 },
                    { title: "CPU%", width: 7, align: "right", padding: 1 },
                    { title: "STATUS", width: 11, align: "center", padding: 1 }
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

            TableView {
                id: servicesTable
                objectName: "servicesTable"
                height: 12
                showOuterBorder: true
                alternatingRows: true
                showColumnGrid: true
                gridStyle: TableView.Single
                columns: [
                    { title: "PORT", width: 6, align: "right", padding: 1 },
                    { title: "SERVICE", width: 13, padding: 1 },
                    { title: "STATE", width: 5, align: "center", padding: 1 }
                ]
                model: [
                    ["22", "sshd", "UP"],
                    ["80", "nginx", "UP"],
                    ["443", "https", "UP"],
                    ["3306", "mysqld", "UP"],
                    ["5432", "postgres", "UP"],
                    ["6379", "redis", "UP"],
                    ["8080", "node", "UP"],
                    ["9090", "prom", "UP"]
                ]
                cellFormat: (row, column, value) => {
                    if (column === 2)
                        return { text: value, color: "green" }
                    return { text: value }
                }
                onRowActivated: (row) => {
                    const rowData = model[row]
                    statusText.text = qsTr("Listening: %1 on port %2").arg(rowData[1]).arg(rowData[0])
                }
            }
        }

        Text {
            id: statusText
            x: 1; y: 20; width: 76
            text: qsTr("Select a process or service")
            color: "cyan"
        }
        Text {
            x: 1; y: 22; width: 76
            text: qsTr("Esc clears the filter, then quits. Q quits outside the filter.")
        }
    }
}
