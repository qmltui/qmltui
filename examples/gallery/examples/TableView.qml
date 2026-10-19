pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property bool gridLines: true
    property string lastEvent: "none"
    readonly property var processRows: [["1042", "api-gateway", "14.2%", "RUNNING"], ["1188", "auth-worker", "84.6%", "RUNNING"], ["1304", "pg-replica", "6.8%", "RUNNING"], ["1420", "cache-sync", "0.0%", "STOPPED"], ["1512", "log-shipper", "32.1%", "RUNNING"], ["1690", "cron-runner", "0.4%", "STOPPED"]]
    readonly property int activeRowIdx: tableView.currentRow >= 0 ? tableView.currentRow : tableView.selectedRow
    readonly property var activeRow: activeRowIdx >= 0 && activeRowIdx < processRows.length ? processRows[activeRowIdx] : ["-", "-", "-", "-"]

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_G) {
                root.gridLines = !root.gridLines;
                event.accepted = true;
            } else if (event.key === Qt.Key_A) {
                tableView.alternatingRows = !tableView.alternatingRows;
                event.accepted = true;
            } else if (event.key === Qt.Key_B) {
                tableView.showOuterBorder = !tableView.showOuterBorder;
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "TableView · Process Monitor"
                    color: "#f8fafc"
                }
                Text {
                    text: "[CORE]"
                    color: "#fb923c"
                }
            }
            titleTrailing: Text {
                text: (tableView.currentRow + 1) + "/" + root.processRows.length
                color: "#8b949e"
            }
            footer: Text {
                text: "↑↓ row · Enter activate · G grid · A alt"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.gridLines ? "grid:on" : "grid:off"
                color: "#58a6ff"
            }
            TableView {
                id: tableView
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                selectedRow: 0
                alternatingRows: true
                showColumnGrid: root.gridLines
                showRowGrid: root.gridLines
                showOuterBorder: true
                stretchLastColumn: true
                selectionKeepsCellColors: true
                headerColor: "#58a6ff"
                textColor: "#c9d1d9"
                gridColor: "#30363d"
                rowBackgroundColor: "#0d1117"
                alternatingRowBackgroundColor: "#161b22"
                selectionBackgroundColor: "#1f2937"
                selectionColor: "#f8fafc"
                columns: [
                    {
                        title: "PID",
                        width: 6,
                        color: "#8b949e"
                    },
                    {
                        title: "NAME",
                        width: 12,
                        color: "#f8fafc"
                    },
                    {
                        title: "CPU%",
                        width: 7,
                        align: "right",
                        headerAlign: "right"
                    },
                    {
                        title: "STATE",
                        width: 8,
                        headerAlign: "center"
                    }
                ]
                model: root.processRows
                cellFormat: (row, column, value) => {
                    if (column === 2) {
                        const cpu = parseFloat(String(value));
                        if (cpu >= 50.0) {
                            return {
                                text: String(value),
                                color: "#f87171",
                                bold: true
                            };
                        }
                        if (cpu >= 20.0) {
                            return {
                                text: String(value),
                                color: "#fbbf24",
                                bold: false
                            };
                        }
                        return {
                            text: String(value),
                            color: "#38bdf8",
                            bold: false
                        };
                    }
                    if (column === 3) {
                        const st = String(value);
                        if (st === "RUNNING") {
                            return {
                                text: st,
                                color: "#2cde85",
                                bold: true
                            };
                        }
                        return {
                            text: st,
                            color: "#8b949e",
                            bold: false
                        };
                    }
                    return {
                        text: String(value)
                    };
                }

                onRowActivated: index => {
                    root.lastEvent = "row#" + index + " " + root.processRows[index][1];
                }
                onHeaderClicked: column => {
                    root.lastEvent = "hdr(" + column + ") " + tableView.columns[column].title;
                }
                Component.onCompleted: tableView.selectedRow = 0
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Inspector"
                color: "#58a6ff"
            }
            footer: Text {
                text: "TableView API"
                color: "#8b949e"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "SELECTED ROW"
                    bold: true
                    color: "#fb923c"
                }

                Text {
                    text: "currentRow: " + root.activeRowIdx
                    color: "#c9d1d9"
                }

                Text {
                    text: "PID:   " + root.activeRow[0]
                    color: "#8b949e"
                }

                Text {
                    text: "Name:  " + root.activeRow[1]
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "CPU%:  " + root.activeRow[2]
                    color: "#38bdf8"
                }

                Text {
                    text: "State: " + root.activeRow[3]
                    color: root.activeRow[3] === "RUNNING" ? "#2cde85" : "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "TELEMETRY"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    width: parent.width
                    text: "Event: " + root.lastEvent
                    color: "#c9d1d9"
                }

                Text {
                    text: "Rows:  " + root.processRows.length + " · Cols: 4"
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "GRID & STYLE"
                    bold: true
                    color: "#58a6ff"
                }

                CheckBox {
                    text: "Grid lines (G)"
                    checked: root.gridLines
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.gridLines = checked;
                    }
                }

                CheckBox {
                    text: "Alt rows (A)"
                    checked: tableView.alternatingRows
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        tableView.alternatingRows = checked;
                    }
                }

                CheckBox {
                    text: "Outer border (B)"
                    checked: tableView.showOuterBorder
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        tableView.showOuterBorder = checked;
                    }
                }
            }
        }
    }
}
