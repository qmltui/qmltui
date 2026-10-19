pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool customColors: true
    property var tabNames: ["Overview", "Processes", "Network", "Storage"]
    readonly property string activeTabName: tabNames[workspaceTabs.currentIndex] || "Overview"

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_1) {
                workspaceTabs.currentIndex = 0;
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                workspaceTabs.currentIndex = 1;
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                workspaceTabs.currentIndex = 2;
                event.accepted = true;
            } else if (event.key === Qt.Key_4) {
                workspaceTabs.currentIndex = 3;
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
                    text: "TabBar"
                    color: "#fbbf24"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.activeTabName
                color: "#2cde85"
            }
            footer: Text {
                text: "Left/Right or 1-4 switch tab"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: (workspaceTabs.currentIndex + 1) + " / 4"
                color: "#fbbf24"
            }
            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                TabBar {
                    id: workspaceTabs

                    x: -1
                    width: parent.width + 2
                    height: 1
                    focus: true
                    tabs: root.tabNames
                    currentIndex: 0
                    textColor: "#8b949e"
                    selectedTextColor: root.customColors ? "#0d1117" : "default"
                    selectionColor: root.customColors ? "#fbbf24" : "default"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#30363d"
                }

                StackLayout {
                    width: parent.width
                    height: 11
                    currentIndex: workspaceTabs.currentIndex

                    Column {
                        spacing: 1

                        Text {
                            text: "Cluster Overview — us-east-prod"
                            color: "#fbbf24"
                            bold: true
                        }

                        Text {
                            text: "Nodes: 12 ready  |  Uptime: 42d 11h"
                            color: "#2cde85"
                        }

                        Text {
                            text: "CPU: 48%  |  RAM: 64.2 / 128 GB"
                            color: "#7ee0ff"
                        }

                        Text {
                            text: "Alerts: 0 critical, 1 warning"
                            color: "#c9d1d9"
                        }
                    }

                    Column {
                        spacing: 1

                        Text {
                            text: "Active Processes (Top CPU)"
                            color: "#2cde85"
                            bold: true
                        }

                        Text {
                            text: "PID 1042  api-gateway      18.4% CPU"
                            color: "#c9d1d9"
                        }

                        Text {
                            text: "PID 1188  postgres-writer  14.1% CPU"
                            color: "#c9d1d9"
                        }

                        Text {
                            text: "PID 1409  redis-server      6.2% CPU"
                            color: "#c9d1d9"
                        }
                    }

                    Column {
                        spacing: 1

                        Text {
                            text: "Network Interfaces & Throughput"
                            color: "#38bdf8"
                            bold: true
                        }

                        Text {
                            text: "eth0   RX: 842 Mbps   TX: 615 Mbps"
                            color: "#2cde85"
                        }

                        Text {
                            text: "wg0    RX: 124 Mbps   TX: 119 Mbps"
                            color: "#7ee0ff"
                        }

                        Text {
                            text: "Active TCP sockets: 4,812 established"
                            color: "#c9d1d9"
                        }
                    }

                    Column {
                        spacing: 1

                        Text {
                            text: "Storage Volumes & IOPS"
                            color: "#c084fc"
                            bold: true
                        }

                        Text {
                            text: "/dev/nvme0n1   412 / 1000 GB (41%)"
                            color: "#2cde85"
                        }

                        Text {
                            text: "/dev/nvme1n1   780 / 1000 GB (78%)"
                            color: "#f9c74f"
                        }

                        Text {
                            text: "Read: 14.2k IOPS  |  Write: 8.9k IOPS"
                            color: "#c9d1d9"
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
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
            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Button {
                    text: "Next Tab"
                    textColor: "#fbbf24"

                    onClicked: {
                        workspaceTabs.currentIndex = (workspaceTabs.currentIndex + 1) % root.tabNames.length;
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "PROPERTIES"
                        color: "#58a6ff"
                        bold: true
                    }

                    CheckBox {
                        checked: root.customColors
                        text: "selectionColor"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.customColors = checked;
                        }
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "TELEMETRY"
                        color: "#58a6ff"
                        bold: true
                    }

                    Text {
                        text: "currentIndex: " + workspaceTabs.currentIndex
                        color: "#fbbf24"
                        bold: true
                    }

                    Text {
                        text: "active: " + root.activeTabName
                        color: "#2cde85"
                    }

                    Text {
                        text: "count: " + root.tabNames.length
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "style: " + (root.customColors ? "Amber Fill" : "Reverse")
                        color: "#c9d1d9"
                    }
                }
            }
        }
    }
}
