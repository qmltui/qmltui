import QtQml
import Qtui
import Qtui.Controls
import Qtui.Extras
import StarterTemplateApp

Rectangle {
    id: page

    color: Theme.bgMain

    // Brand banner
    Rectangle {
        id: banner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 9
        color: Theme.bgHeader
        border.color: Theme.borderNormal

        BigText {
            id: brandText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 1
            text: "BOOT TO QTUI"
            color: Theme.qtGreen
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: brandText.bottom
            text: "Terminal UI device setup for embedded Linux · Built with Qt " + WifiManager.qtVersion
            color: Theme.textSecondary
        }
    }

    // Device information
    Panel {
        id: devicePanel
        anchors.top: banner.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Math.floor(parent.width / 3)
        color: Theme.bgPanel
        border.color: Theme.borderNormal
        title: "Device"
        titleColor: Theme.qtGreen

        Column {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 1

            Repeater {
                model: [
                    { label: "Hostname", value: WifiManager.hostname },
                    { label: "Uptime", value: WifiManager.uptime },
                    { label: "Memory", value: WifiManager.memoryInfo },
                    { label: "CPU Load", value: WifiManager.cpuLoad },
                ]

                Row {
                    spacing: 1

                    Text {
                        text: modelData.label + ":"
                        color: Theme.textSecondary
                    }

                    Text {
                        text: modelData.value
                        color: Theme.textPrimary
                        bold: true
                    }
                }
            }
        }
    }

    // Software information
    Panel {
        id: softwarePanel
        anchors.top: banner.bottom
        anchors.bottom: parent.bottom
        anchors.left: devicePanel.right
        width: Math.floor(parent.width / 3)
        color: Theme.bgPanel
        border.color: Theme.borderNormal
        title: "Software"
        titleColor: Theme.qtGreen

        Column {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 1

            Repeater {
                model: [
                    { label: "Application", value: "Boot to Qtui Setup 1.0" },
                    { label: "Qt", value: WifiManager.qtVersion },
                    { label: "UI Toolkit", value: "Qtui (terminal rendering)" },
                ]

                Row {
                    spacing: 1

                    Text {
                        text: modelData.label + ":"
                        color: Theme.textSecondary
                    }

                    Text {
                        text: modelData.value
                        color: Theme.textPrimary
                        bold: true
                    }
                }
            }
        }
    }

    // Network summary
    Panel {
        anchors.top: banner.bottom
        anchors.bottom: parent.bottom
        anchors.left: softwarePanel.right
        anchors.right: parent.right
        color: Theme.bgPanel
        border.color: Theme.borderNormal
        title: "Network"
        titleColor: Theme.qtGreen

        Column {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 1

            Text {
                text: WifiManager.connected ? "Wi-Fi: " + Theme.check + " " + WifiManager.currentSsid : "Wi-Fi: Not connected"
                color: WifiManager.connected ? Theme.qtGreen : Theme.statusWarn
                bold: true
            }

            Repeater {
                model: WifiManager.allInterfaces

                Column {
                    Text {
                        text: modelData.displayName
                        color: Theme.textSecondary
                    }

                    Text {
                        visible: modelData.ip !== "-"
                        text: "  v4 " + modelData.ip
                        color: Theme.qtGreen
                        bold: true
                    }

                    Text {
                        visible: modelData.ip6 !== "-"
                        text: "  v6 " + modelData.ip6
                        color: modelData.hasGlobalIp6 ? Theme.qtGreen : Theme.textSecondary
                        bold: modelData.hasGlobalIp6
                    }
                }
            }
        }
    }
}
