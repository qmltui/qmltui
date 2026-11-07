pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import PySideDemo

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (filterInput.text.length > 0) {
                filterInput.text = "";
            } else {
                Qt.quit();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Q && !filterInput.activeFocus) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Panel {
        anchors {
            fill: parent
            margins: 1
        }
        border {
            style: Border.Single
            color: "cyan"
        }
        title: Text {
            text: " PySide6 + QmlTui Service Monitor "
            color: "cyan"
            bold: true
        }
        footer: Text {
            text: " Enter: Toggle | Click header: Sort | Esc: Clear/Quit | Q: Quit "
            dim: true
        }

        Column {
            anchors {
                fill: parent
                margins: 1
            }
            spacing: 1

            Row {
                spacing: 1

                Text {
                    text: "Filter:"
                    bold: true
                }

                TextInput {
                    id: filterInput
                    objectName: "filterInput"
                    width: 24
                    height: 1
                    placeholderText: "service name..."
                }

                Text {
                    text: services.sortLabel
                    color: "yellow"
                }
            }

            TableView {
                id: serviceTable
                objectName: "serviceTable"
                focus: true
                height: 11
                showOuterBorder: true
                showColumnGrid: true
                alternatingRows: true
                gridStyle: TableView.Single
                model: services
                columns: [
                    {
                        title: "PID",
                        width: 8,
                        padding: 1
                    },
                    {
                        title: "SERVICE",
                        width: 20,
                        padding: 1
                    },
                    {
                        title: "CPU%",
                        width: 8,
                        align: "right",
                        padding: 1
                    },
                    {
                        title: "STATUS",
                        width: 12,
                        align: "center",
                        padding: 1
                    }
                ]
                onHeaderClicked: column => services.headerClicked(column)
                onRowActivated: row => services.toggleRow(row)
                cellFormat: (row, column, value) => {
                    if (column === 3) {
                        const cellColor = value === "RUNNING" ? "green" : (value === "DEGRADED" ? "yellow" : "red");
                        return {
                            text: value,
                            color: cellColor
                        };
                    }
                    return {
                        text: value
                    };
                }
            }

            Text {
                id: statusBanner
                objectName: "statusBanner"
                text: services.statusMessage
                color: "cyan"
            }
        }
    }

    ServiceController {
        id: services
        filterText: filterInput.text
    }
}
