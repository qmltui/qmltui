pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls
import Qtui.Extras
import ContainerFleetApp

Terminal {
    id: root
    color: "#0d0e15" // Deep dark pastel canvas background

    // Instantiate C++ QAbstractListModel with tableData extension
    FleetListModel {
        id: fleetModel
    }

    property bool showLogs: false

    // Soft Pastel Palette (Catppuccin Mocha / Nord Inspired)
    readonly property string colorHeaderBg:    "#181825"
    readonly property string colorCardBg:      "#14141f"
    readonly property string colorCardAltBg:   "#11111b"
    readonly property string colorCardSelBg:   "#272738"
    readonly property string colorTextPrimary: "#cdd6f4"
    readonly property string colorTextMuted:   "#6c7086"
    readonly property string colorAccentCyan:  "#89dceb"
    readonly property string colorAccentGreen: "#a6e3a1"
    readonly property string colorAccentRed:   "#f38ba8"
    readonly property string colorAccentYellow: "#f9e2af"
    readonly property string colorAccentPurple: "#cba6f7"

    // MANDATORY QTUI RULE: Explicit key handling for Q/Escape to quit
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || (event.key === Qt.Key_Escape && searchInput.text === "")) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            searchInput.text = ""
            fleetModel.searchQuery = ""
            tableView.focus = true
            event.accepted = true
        } else if (event.key === Qt.Key_Slash) {
            searchInput.focus = true
            event.accepted = true
        } else if (event.key === Qt.Key_1) {
            fleetModel.activeFilter = 0
            tableView.selectedRow = 0
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            fleetModel.activeFilter = 1
            tableView.selectedRow = 0
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            fleetModel.activeFilter = 2
            tableView.selectedRow = 0
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            fleetModel.toggleNodeStatus(tableView.selectedRow)
            event.accepted = true
        } else if (event.key === Qt.Key_A) {
            fleetModel.restartAllCritical()
            event.accepted = true
        } else if (event.key === Qt.Key_L) {
            root.showLogs = !root.showLogs
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // --- 1. UNIFIED SLEEK TOP CONTROL HEADER ---
        Panel {
            width: parent.width
            height: 3
            color: root.colorHeaderBg
            border.color: "#272738"
            title: "⚡ DOCKER FLEET MONITOR  |  Total: " + fleetModel.totalNodes + "  Running: " + fleetModel.runningNodes + "  Critical: " + fleetModel.alertNodes
            titleColor: root.colorAccentCyan

            Row {
                x: 1; y: 1
                width: parent.width - 2
                spacing: 4

                // Left: Borderless Filter Tabs
                TabHeader {
                    id: filterTabs
                    currentIndex: fleetModel.activeFilter
                    activeTabBgColor: root.colorAccentCyan
                    activeTabColor: "#11111b"
                    inactiveTabBgColor: "transparent"
                    inactiveTabColor: root.colorTextMuted
                    tabs: [
                        " 1: ALL (" + fleetModel.totalNodes + ") ",
                        " 2: RUNNING (" + fleetModel.runningNodes + ") ",
                        " 3: CRITICAL (" + fleetModel.alertNodes + ") "
                    ]
                    onCurrentIndexChanged: {
                        if (fleetModel.activeFilter !== filterTabs.currentIndex) {
                            fleetModel.activeFilter = filterTabs.currentIndex
                            tableView.selectedRow = 0
                        }
                    }
                }

                // Right: Clean Flat Search Input
                Row {
                    spacing: 1

                    Text {
                        text: "🔍 [/] Search: "
                        color: searchInput.focus ? root.colorAccentYellow : root.colorTextMuted
                        bold: searchInput.focus
                    }

                    Rectangle {
                        width: 20
                        height: 1
                        border.style: "none"
                        color: searchInput.focus ? "#2a2b3d" : "#14141f"

                        TextInput {
                            id: searchInput
                            width: 18
                            text: ""
                            color: root.colorTextPrimary
                            onTextChanged: fleetModel.searchQuery = searchInput.text
                            onAccepted: tableView.focus = true
                        }
                    }
                }
            }
        }

        // --- 2. NATIVE TABLEVIEW WITH HEADER SORTING ---
        Rectangle {
            width: parent.width
            height: Math.max(6, parent.height - (root.showLogs ? 11 : 7))
            border.style: "single"
            border.color: "#272738"

            TableView {
                id: tableView
                focus: true
                anchors.fill: parent
                anchors.margins: 1
                showGrid: true
                headerColor: root.colorAccentCyan
                headerBgColor: "#181825"
                selectionColor: root.colorAccentYellow
                selectionBgColor: root.colorCardSelBg
                alternatingRows: true
                alternatingRowBgColor: root.colorCardAltBg
                rowBgColor: root.colorCardBg

                columns: [
                    { title: "NODE NAME", width: 19, align: "left" },
                    { title: "IP ADDRESS", width: 14, align: "left" },
                    { title: "STATUS", width: 12, align: "center" },
                    { title: "CPU LOAD", width: 17, align: "left" },
                    { title: "MEMORY", width: 17, align: "left" },
                    { title: "TAGS & SERVICES", width: 22, align: "left" }
                ]

                model: fleetModel.tableData

                cellFormat: (row, col, value) => {
                    if (col === 2) {
                        if (value === "RUNNING") return { text: "✔ RUNNING", color: root.colorAccentGreen, bold: true }
                        if (value === "CRITICAL") return { text: "✖ CRITICAL", color: root.colorAccentRed, bold: true }
                        if (value === "STANDBY") return { text: "⏸ STANDBY", color: root.colorAccentYellow, bold: true }
                    } else if (col === 3) {
                        let isHigh = value.indexOf("80%") !== -1 || value.indexOf("90%") !== -1 || value.indexOf("100%") !== -1
                        return { text: value, color: isHigh ? root.colorAccentRed : root.colorAccentCyan, bold: isHigh }
                    } else if (col === 4) {
                        let isHigh = value.indexOf("80%") !== -1 || value.indexOf("90%") !== -1 || value.indexOf("100%") !== -1
                        return { text: value, color: isHigh ? root.colorAccentRed : root.colorAccentGreen, bold: isHigh }
                    } else if (col === 5) {
                        return { text: value, color: root.colorAccentPurple }
                    }
                    return { text: value }
                }
            }
        }

        // --- 3. BORDERED INSPECTOR DRAWER PANEL WITH SPARKLINE ---
        Panel {
            width: parent.width
            height: 4
            color: "#181825"
            border.color: "#272738"
            title: "🔍 NODE INSPECTOR & REAL-TIME TELEMETRY"
            titleColor: root.colorAccentCyan

            Column {
                x: 1; y: 1
                spacing: 0

                Row {
                    spacing: 2
                    Text {
                        text: fleetModel.getNodeName(tableView.selectedRow) !== "" ? 
                              fleetModel.getNodeName(tableView.selectedRow) + " (" + fleetModel.getNodeId(tableView.selectedRow) + ")" : "Node Selection Active"
                        color: root.colorTextPrimary
                        bold: true
                    }
                    Text {
                        text: " | CPU Sparkline: [" + fleetModel.getSparkline(tableView.selectedRow) + "]"
                        color: root.colorAccentYellow
                        bold: true
                    }
                }
                Text {
                    text: fleetModel.getNodeName(tableView.selectedRow) !== "" ?
                          "Status: " + fleetModel.getNodeStatus(tableView.selectedRow) + 
                          " | IP: " + fleetModel.getNodeIp(tableView.selectedRow) + 
                          " | CPU Load: " + (fleetModel.getNodeCpu(tableView.selectedRow) * 10) + "%" +
                          " | Memory: " + (fleetModel.getNodeMem(tableView.selectedRow) * 10) + "%" : ""
                    color: root.colorTextMuted
                }
            }
        }

        // --- 4. OPTIONAL LIVE DOCKER LOG DRAWER ---
        Panel {
            visible: root.showLogs
            width: parent.width
            height: 4
            color: "#11111b"
            border.color: "#272738"
            title: "📜 LIVE CONTAINER LOG STREAM (" + fleetModel.getNodeName(tableView.selectedRow) + ")"
            titleColor: root.colorAccentPurple

            Column {
                x: 1; y: 1
                spacing: 0

                Repeater {
                    model: fleetModel.getLogs(tableView.selectedRow)
                    delegate: Text {
                        required property string modelData
                        text: "  > " + modelData
                        color: modelData.indexOf("ALERT") !== -1 ? root.colorAccentRed :
                               modelData.indexOf("WARN") !== -1 ? root.colorAccentYellow : root.colorTextMuted
                    }
                }
            }
        }

        // --- 5. COMPLETE FOOTER SHORTCUT LEGEND ---
        Panel {
            width: parent.width
            height: 3
            color: "#0d1c2d"
            border.color: "#273647"
            title: "SHORTCUTS & COMMANDS"
            titleColor: root.colorAccentGreen

            Row {
                x: 1; y: 1; spacing: 2
                Text {
                    text: "[1-3] Filter  [/] Search  [Click/Nav Headers] Sort  [R] Restart Node  [A] Fix All  [L] Logs  [Q/Esc] Quit"
                    color: root.colorAccentGreen
                    bold: true
                }
            }
        }
    }
}
