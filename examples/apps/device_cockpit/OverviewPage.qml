pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls
import Qtui.Extras
import Qtui.Examples.DeviceCockpit

Rectangle {
    id: page
    color: "transparent"

    readonly property var info: CockpitBackend.systemInfo

    Row {
        anchors.fill: parent
        spacing: 1

        // LEFT COLUMN: identity and network
        Column {
            width: Math.floor((parent.width - 1) / 2)
            height: parent.height
            spacing: 0

            Panel {
                title: "SYSTEM"
                titleColor: Theme.textAccent
                width: parent.width
                height: 8
                color: Theme.bgPanel
                border.color: Theme.borderNormal

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 2
                    spacing: 0

                    Row {
                        spacing: 1
                        Text { text: "Hostname:"; color: Theme.textSecondary }
                        Text { text: page.info.hostname; color: Theme.textPrimary; bold: true }
                    }
                    Row {
                        spacing: 1
                        Text { text: "OS:      "; color: Theme.textSecondary }
                        Text { text: page.info.osName; color: Theme.textPrimary }
                    }
                    Row {
                        spacing: 1
                        Text { text: "Kernel:  "; color: Theme.textSecondary }
                        Text { text: page.info.kernelVersion; color: Theme.textPrimary }
                    }
                    Row {
                        spacing: 1
                        Text { text: "Uptime:  "; color: Theme.textSecondary }
                        Text { text: Theme.formatUptime(page.info.uptimeSeconds); color: Theme.textPrimary }
                    }
                    Row {
                        spacing: 1
                        Text { text: "Load:    "; color: Theme.textSecondary }
                        Text { text: page.info.loadAverage.toFixed(2); color: Theme.textPrimary }
                    }
                }
            }

            Panel {
                title: "NETWORK"
                titleColor: Theme.textAccent
                width: parent.width
                height: parent.height - 8
                color: Theme.bgPanel
                border.color: Theme.borderNormal

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 2
                    spacing: 0

                    Repeater {
                        model: page.info.interfaces

                        Row {
                            id: nicRow
                            required property var modelData
                            spacing: 1

                            Text {
                                text: (nicRow.modelData.name + "      ").substring(0, 7)
                                color: Theme.textPrimary
                                bold: true
                            }
                            StatusBadge {
                                status: nicRow.modelData.up ? "success" : "error"
                                text: nicRow.modelData.up ? "UP" : "DOWN"
                                showIcon: false
                            }
                            Text {
                                text: nicRow.modelData.ipv4 !== "" ? nicRow.modelData.ipv4 : "no address"
                                color: nicRow.modelData.ipv4 !== "" ? Theme.textAccent : Theme.textDim
                            }
                        }
                    }
                }
            }
        }

        // RIGHT COLUMN: live stats and storage
        Column {
            width: parent.width - Math.floor((parent.width - 1) / 2) - 1
            height: parent.height
            spacing: 0

            Panel {
                title: "CPU"
                titleColor: Theme.textAccent
                width: parent.width
                height: 8
                color: Theme.bgPanel
                border.color: Theme.borderNormal

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 2
                    spacing: 0

                    Gauge {
                        width: parent.width - 1
                        height: 1
                        value: page.info.cpuLoadPct
                        label: "load"
                        gaugeColor: page.info.cpuLoadPct > 85 ? Theme.statusError : Theme.statusOk
                    }
                    Sparkline {
                        width: parent.width - 1
                        height: 3
                        values: page.info.cpuHistory
                        minVal: 0
                        maxVal: 100
                        color: Theme.textAccent
                    }
                    Row {
                        spacing: 2
                        Text {
                            text: "Temp: " + (page.info.cpuTempC >= 0
                                              ? page.info.cpuTempC.toFixed(1) + "°C"
                                              : "n/a")
                            color: page.info.cpuTempC > 75 ? Theme.textDanger : Theme.textSecondary
                        }
                        Text {
                            text: "Load avg: " + page.info.loadAverage.toFixed(2)
                            color: Theme.textSecondary
                        }
                    }
                }
            }

            Panel {
                title: "MEMORY"
                titleColor: Theme.textAccent
                width: parent.width
                height: 3
                color: Theme.bgPanel
                border.color: Theme.borderNormal

                Row {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 2
                    spacing: 1

                    Gauge {
                        width: Math.max(10, parent.width - 22)
                        height: 1
                        value: page.info.memUsedMb
                        maxValue: Math.max(1, page.info.memTotalMb)
                        gaugeColor: Theme.statusOk
                    }
                    Text {
                        text: Theme.formatMb(page.info.memUsedMb) + " / "
                              + Theme.formatMb(page.info.memTotalMb)
                        color: Theme.textSecondary
                    }
                }
            }

            Panel {
                title: "STORAGE"
                titleColor: Theme.textAccent
                width: parent.width
                height: parent.height - 8 - 3
                color: Theme.bgPanel
                border.color: Theme.borderNormal

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.leftMargin: 2
                    spacing: 0

                    Repeater {
                        model: page.info.disks

                        Row {
                            id: diskRow
                            required property var modelData
                            spacing: 1

                            Text {
                                text: (diskRow.modelData.mount + "          ").substring(0, 10)
                                color: Theme.textPrimary
                                bold: true
                            }
                            Text {
                                text: Theme.makeBar(
                                          diskRow.modelData.usedMb / Math.max(1, diskRow.modelData.totalMb), 12)
                                color: diskRow.modelData.usedMb / Math.max(1, diskRow.modelData.totalMb) > 0.9
                                       ? Theme.statusError : Theme.statusOk
                            }
                            Text {
                                text: Theme.formatMb(diskRow.modelData.usedMb) + " / "
                                      + Theme.formatMb(diskRow.modelData.totalMb)
                                color: Theme.textSecondary
                            }
                        }
                    }
                }
            }
        }
    }
}
