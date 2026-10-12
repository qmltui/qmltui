pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    width: 80
    height: 24

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        id: tableDemo
        x: 2
        y: 1
        property string status: "Select a row or click a header"
        spacing: 1

        Text {
            id: statusLabel
            text: tableDemo.status
            color: "#fb923c"
        }

        TableView {
            id: table
            objectName: "table"
            height : 10
            focus: true
            alternatingRows: true
            showColumnGrid: true
            showRowGrid: true
            showOuterBorder: true
            headerColor: "#fb923c"
            columns: [
                {
                    title: "PID",
                    width: 7,
                    align: "right",
                    padding: 1
                },
                {
                    title: "NAME",
                    width: 22,
                    padding: 1
                },
                {
                    title: "CPU%",
                    width: 8,
                    align: "right",
                    padding: 1
                },
                {
                    title: "STATE",
                    width: 12,
                    align: "center",
                    padding: 1
                }
            ]
            model: [[1, "systemd", "0.1", "sleeping"], [412, "qmltui_gallery", "3.4", "running"], [977, "sshd", "0.0", "sleeping"], [1820, "postgres", "12.8", "running"], [2201, "node", "41.2", "running"], [3090, "zsh", "0.0", "stopped"]]
            cellFormat: (row, column, value) => {
                if (column === 3)
                    return {
                        text: value,
                        color: value === "running" ? "#65e6a0" : value === "stopped" ? "#f87171" : "#6e7b8b"
                    };
                if (column === 2 && parseFloat(value) > 10)
                    return {
                        text: value,
                        color: "#fbbf24",
                        bold: true
                    };
                return {
                    text: value
                };
            }
            onRowActivated: row => tableDemo.status = "Activated row " + row + ": " + model[row][1]
            onHeaderClicked: column => tableDemo.status = "Header " + column + " clicked (sorting is the model's job)"
        }
    }
}
