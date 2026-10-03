pragma ComponentBehavior: Bound

import QtQml 2.15
import Qtui
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
        anchors.margins: 1
        spacing: 1

        // --- 1. SLEEK BORDERLESS HEADER BANNER ---
        Rectangle {
            width: parent.width
            height: 2
            border.style: "none"
            color: root.colorHeaderBg

            Row {
                x: 2; y: 0
                spacing: 3
                Text { text: "⚡ DOCKER & CLOUD FLEET MONITOR"; bold: true; color: root.colorAccentCyan }
                Text { 
                    text: "Total: " + fleetModel.totalNodes + "  |  Running: " + fleetModel.runningNodes + "  |  Critical: " + fleetModel.alertNodes
                    color: root.colorTextPrimary
                }
            }
        }

        // --- 2. SEARCH BAR & FILTER TABS & CONTROL BAR ---
        Row {
            spacing: 1

            // Filter Tabs
            Rectangle {
                color: fleetModel.activeFilter === 0 ? root.colorAccentCyan : "#1e1e2e"
                border.style: "none"
                height: 1
                Text { 
                    text: " 1: ALL (" + fleetModel.totalNodes + ") "
                    bold: fleetModel.activeFilter === 0
                    color: fleetModel.activeFilter === 0 ? "#11111b" : root.colorTextMuted
                }
            }
            Rectangle {
                color: fleetModel.activeFilter === 1 ? root.colorAccentGreen : "#1e1e2e"
                border.style: "none"
                height: 1
                Text { 
                    text: " 2: RUNNING (" + fleetModel.runningNodes + ") "
                    bold: fleetModel.activeFilter === 1
                    color: fleetModel.activeFilter === 1 ? "#11111b" : root.colorTextMuted
                }
            }
            Rectangle {
                color: fleetModel.activeFilter === 2 ? root.colorAccentRed : "#1e1e2e"
                border.style: "none"
                height: 1
                Text { 
                    text: " 3: CRITICAL (" + fleetModel.alertNodes + ") "
                    bold: fleetModel.activeFilter === 2
                    color: fleetModel.activeFilter === 2 ? "#11111b" : root.colorTextMuted
                }
            }

            // Interactive Search Input
            Rectangle {
                color: searchInput.focus ? "#2a2b3d" : "#1e1e2e"
                border.style: "none"
                height: 1
                Row {
                    spacing: 0
                    Text { text: " 🔍 Filter: "; color: root.colorAccentYellow; bold: true }
                    TextInput {
                        id: searchInput
                        text: ""
                        color: root.colorTextPrimary
                        onTextChanged: fleetModel.searchQuery = searchInput.text
                        onAccepted: listView.focus = true
                    }
                }
            }

            // Sort indicator button
            Rectangle {
                color: "#1e1e2e"
                border.style: "none"
                height: 1
                Text {
                    text: " [S] Sort: " + (fleetModel.sortMode === 1 ? "CPU ↓" : fleetModel.sortMode === 2 ? "RAM ↓" : fleetModel.sortMode === 3 ? "Status" : "Name") + " "
                    color: root.colorAccentPurple
                    bold: true
                }
            }

            Text {
                text: " [A] Fix All [L] Logs [Q] Quit"
                color: root.colorTextMuted
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

        // --- 4. MAIN BORDERLESS LISTVIEW ---
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

                    // Active Item Highlight Pillar
                    Text {
                        x: 0; y: 0
                        text: ListView.isCurrentItem ? "▌" : " "
                        color: root.colorAccentYellow
                        bold: true
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

                    // Line 2: Soft Pastel CPU & RAM Gauges (Optimized Native TUI Bars)
                    Row {
                        x: 5; y: 1
                        spacing: 0
                        Text { text: "CPU: ["; color: root.colorTextMuted }
                        Text { text: "█".repeat(itemDelegate.cpu); color: itemDelegate.cpu > 7 ? root.colorAccentRed : root.colorAccentCyan }
                        Text { text: "░".repeat(Math.max(0, 10 - itemDelegate.cpu)); color: "#313244" }
                        Text { text: "] " + (itemDelegate.cpu * 10) + "%"; color: root.colorTextPrimary }
                    }

                    Row {
                        x: 33; y: 1
                        spacing: 0
                        Text { text: "RAM: ["; color: root.colorTextMuted }
                        Text { text: "█".repeat(itemDelegate.mem); color: itemDelegate.mem > 7 ? root.colorAccentRed : root.colorAccentGreen }
                        Text { text: "░".repeat(Math.max(0, 10 - itemDelegate.mem)); color: "#313244" }
                        Text { text: "] " + (itemDelegate.mem * 10) + "%"; color: root.colorTextPrimary }
                    }
                }
            }
        }

        // --- 5. BORDERLESS INSPECTOR DRAWER PANEL WITH SPARKLINE ---
        Rectangle {
            width: parent.width
            height: 4
            border.style: "none"
            color: "#181825"

            Column {
                x: 2; y: 0
                spacing: 0

                Row {
                    spacing: 2
                    Text { text: "INSPECTOR:"; bold: true; color: root.colorAccentCyan }
                    Text {
                        text: fleetModel.getNodeName(listView.currentIndex) !== "" ? 
                              fleetModel.getNodeName(listView.currentIndex) + " (" + fleetModel.getNodeId(listView.currentIndex) + ")" : "No Selection"
                        color: root.colorTextPrimary
                        bold: true
                    }
                    Text {
                        text: "  CPU Sparkline: [" + fleetModel.getSparkline(listView.currentIndex) + "]"
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
                Text {
                    text: "Log Stream: [OK] Heartbeat sync active | [METRICS] CPU=" + 
                          (fleetModel.getNodeCpu(listView.currentIndex) * 10) + "% | RAM=" + 
                          (fleetModel.getNodeMem(listView.currentIndex) * 10) + "%"
                    color: root.colorAccentGreen
                }
            }
        }

        // --- 6. OPTIONAL LIVE DOCKER LOG DRAWER ---
        Rectangle {
            visible: root.showLogs
            width: parent.width
            height: 4
            border.style: "none"
            color: "#11111b"

            Column {
                x: 2; y: 0
                spacing: 0

                Text { text: "📜 LIVE CONTAINER LOG STREAM (" + fleetModel.getNodeName(listView.currentIndex) + ")"; bold: true; color: root.colorAccentPurple }
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
    }
}
