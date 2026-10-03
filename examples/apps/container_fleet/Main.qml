pragma ComponentBehavior: Bound

import QtQuick
import Qtui
import Qtui.Controls
import Qtui.Extras
import ContainerFleetApp

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    color: "#0d0e15" // Deep dark pastel canvas background

    // Instantiate C++ QAbstractListModel
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
            listView.focus = true
            event.accepted = true
        } else if (event.key === Qt.Key_Slash) {
            searchInput.focus = true
            event.accepted = true
        } else if (event.key === Qt.Key_1) {
            fleetModel.activeFilter = 0
            listView.currentIndex = 0
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            fleetModel.activeFilter = 1
            listView.currentIndex = 0
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            fleetModel.activeFilter = 2
            listView.currentIndex = 0
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            fleetModel.toggleNodeStatus(listView.currentIndex)
            event.accepted = true
        } else if (event.key === Qt.Key_A) {
            fleetModel.restartAllCritical()
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            fleetModel.cycleSortMode()
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

        // --- 1. SLEEK HEADER BANNER ---
        Panel {
            width: parent.width
            height: 3
            color: root.colorHeaderBg
            border.color: "#272738"
            title: "⚡ DOCKER & CLOUD FLEET MONITOR"
            titleColor: root.colorAccentCyan

            Row {
                x: 1; y: 1
                spacing: 3
                Text { 
                    text: "Total Nodes: " + fleetModel.totalNodes + "  |  Running: " + fleetModel.runningNodes + "  |  Critical: " + fleetModel.alertNodes
                    color: root.colorTextPrimary
                    bold: true
                }
            }
        }

        // --- 2. FILTER TABS & CONTROL BAR ---
        Row {
            width: parent.width
            height: 3
            spacing: 1

            // Filter Tabs
            TabHeader {
                id: filterTabs
                currentIndex: fleetModel.activeFilter
                tabs: [
                    " 1: ALL (" + fleetModel.totalNodes + ") ",
                    " 2: RUNNING (" + fleetModel.runningNodes + ") ",
                    " 3: CRITICAL (" + fleetModel.alertNodes + ") "
                ]
                onCurrentIndexChanged: {
                    if (fleetModel.activeFilter !== filterTabs.currentIndex) {
                        fleetModel.activeFilter = filterTabs.currentIndex
                        listView.currentIndex = 0
                    }
                }
            }

            // Interactive Search Input
            Panel {
                width: 24
                height: 3
                color: searchInput.focus ? "#2a2b3d" : "#1e1e2e"
                border.color: searchInput.focus ? root.colorAccentYellow : "#272738"
                title: "SEARCH"
                titleColor: root.colorAccentYellow

                Row {
                    x: 1; y: 1
                    spacing: 0
                    Text { text: "🔍 "; color: root.colorAccentYellow }
                    TextInput {
                        id: searchInput
                        width: 18
                        text: ""
                        color: root.colorTextPrimary
                        onTextChanged: fleetModel.searchQuery = searchInput.text
                        onAccepted: listView.focus = true
                    }
                }
            }

            // Sort Mode Indicator Button
            Button {
                label: " [S] SORT: " + (fleetModel.sortMode === 1 ? "CPU ↓" : fleetModel.sortMode === 2 ? "RAM ↓" : fleetModel.sortMode === 3 ? "STATUS" : "NAME") + " "
                normalColor: "#1e1e2e"
                textColor: root.colorAccentPurple
                normalBorderStyle: "single"
                onClicked: fleetModel.cycleSortMode()
            }
        }

        // --- 3. CLEAN TABLE COLUMN HEADERS ---
        Rectangle {
            width: parent.width
            height: 1
            border.style: "none"
            color: "#181825"

            Item {
                anchors.fill: parent

                Text { x: 3;  text: "NODE NAME"; bold: true; color: root.colorAccentCyan }
                Text { x: 24; text: "IP ADDRESS"; bold: true; color: root.colorAccentCyan }
                Text { x: 40; text: "STATUS"; bold: true; color: root.colorAccentCyan }
                Text { x: 56; text: "SERVICES / TAGS"; bold: true; color: root.colorAccentCyan }
            }
        }

        // --- 4. MAIN LISTVIEW ---
        ListView {
            id: listView
            width: parent.width
            height: Math.max(5, parent.height - (root.showLogs ? 15 : 11))
            clip: true
            spacing: 0
            model: fleetModel
            focus: true

            delegate: Rectangle {
                id: itemDelegate
                required property int index
                required property string nodeId
                required property string name
                required property string status
                required property int cpu
                required property int mem
                required property string ip
                required property string tags

                width: listView.width
                height: 3
                border.style: "none"
                color: ListView.isCurrentItem ? root.colorCardSelBg : 
                       (itemDelegate.index % 2 === 1 ? root.colorCardAltBg : root.colorCardBg)

                Item {
                    anchors.fill: parent

                    // Active Item Full 3-Line Selection Bar
                    Rectangle {
                        x: 0; y: 0
                        width: 1
                        height: parent.height
                        color: ListView.isCurrentItem ? root.colorAccentYellow : "transparent"
                        border.style: "none"
                    }

                    // Line 1: Identity, IP Address, Status Badge, Tags
                    Text { 
                        x: 3; y: 0
                        text: itemDelegate.name 
                        bold: ListView.isCurrentItem
                        color: ListView.isCurrentItem ? root.colorAccentYellow : root.colorTextPrimary
                    }

                    Text { 
                        x: 24; y: 0
                        text: itemDelegate.ip 
                        color: root.colorTextMuted
                    }

                    // Borderless Status Pill Badge
                    Rectangle {
                        x: 40; y: 0
                        border.style: "none"
                        color: itemDelegate.status === "RUNNING" ? root.colorAccentGreen :
                               itemDelegate.status === "CRITICAL" ? root.colorAccentRed : root.colorAccentYellow
                        height: 1
                        Text {
                            text: " " + itemDelegate.status + " "
                            bold: true
                            color: "#11111b"
                        }
                    }

                    Text { 
                        x: 56; y: 0
                        text: itemDelegate.tags
                        color: root.colorAccentPurple
                    }

                    // Line 2: High-Contrast CPU & RAM Gauge Bars
                    Row {
                        x: 3; y: 1
                        spacing: 0
                        Text { text: "CPU: ["; color: root.colorTextMuted }
                        Text { text: "█".repeat(itemDelegate.cpu); color: itemDelegate.cpu >= 8 ? root.colorAccentRed : root.colorAccentCyan }
                        Text { text: "░".repeat(Math.max(0, 10 - itemDelegate.cpu)); color: "#313244" }
                        Text { text: "] " + (itemDelegate.cpu * 10) + "%"; color: itemDelegate.cpu >= 8 ? root.colorAccentRed : root.colorTextPrimary; bold: itemDelegate.cpu >= 8 }
                    }

                    Row {
                        x: 31; y: 1
                        spacing: 0
                        Text { text: "RAM: ["; color: root.colorTextMuted }
                        Text { text: "█".repeat(itemDelegate.mem); color: itemDelegate.mem >= 8 ? root.colorAccentRed : root.colorAccentGreen }
                        Text { text: "░".repeat(Math.max(0, 10 - itemDelegate.mem)); color: "#313244" }
                        Text { text: "] " + (itemDelegate.mem * 10) + "%"; color: itemDelegate.mem >= 8 ? root.colorAccentRed : root.colorTextPrimary; bold: itemDelegate.mem >= 8 }
                    }
                }
            }
        }

        // --- 5. BORDERED INSPECTOR DRAWER PANEL WITH SPARKLINE ---
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
                        text: fleetModel.getNodeName(listView.currentIndex) !== "" ? 
                              fleetModel.getNodeName(listView.currentIndex) + " (" + fleetModel.getNodeId(listView.currentIndex) + ")" : "Node Selection Active"
                        color: root.colorTextPrimary
                        bold: true
                    }
                    Text {
                        text: " | CPU Sparkline: [" + fleetModel.getSparkline(listView.currentIndex) + "]"
                        color: root.colorAccentYellow
                        bold: true
                    }
                }
                Text {
                    text: fleetModel.getNodeName(listView.currentIndex) !== "" ?
                          "Status: " + fleetModel.getNodeStatus(listView.currentIndex) + 
                          " | IP: " + fleetModel.getNodeIp(listView.currentIndex) + 
                          " | CPU Load: " + (fleetModel.getNodeCpu(listView.currentIndex) * 10) + "%" +
                          " | Memory: " + (fleetModel.getNodeMem(listView.currentIndex) * 10) + "%" : ""
                    color: root.colorTextMuted
                }
            }
        }

        // --- 6. OPTIONAL LIVE DOCKER LOG DRAWER ---
        Panel {
            visible: root.showLogs
            width: parent.width
            height: 4
            color: "#11111b"
            border.color: "#272738"
            title: "📜 LIVE CONTAINER LOG STREAM (" + fleetModel.getNodeName(listView.currentIndex) + ")"
            titleColor: root.colorAccentPurple

            Column {
                x: 1; y: 1
                spacing: 0

                Repeater {
                    model: fleetModel.getLogs(listView.currentIndex)
                    delegate: Text {
                        required property string modelData
                        text: "  > " + modelData
                        color: modelData.indexOf("ALERT") !== -1 ? root.colorAccentRed :
                               modelData.indexOf("WARN") !== -1 ? root.colorAccentYellow : root.colorTextMuted
                    }
                }
            }
        }

        // --- 7. COMPLETE FOOTER SHORTCUT LEGEND ---
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
                    text: "[1-3] Filter  [/] Search  [S] Sort  [R] Restart Node  [A] Fix All Critical  [L] Toggle Logs  [Q/Esc] Quit"
                    color: root.colorAccentGreen
                    bold: true
                }
            }
        }
    }
}
