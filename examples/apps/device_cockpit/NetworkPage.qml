pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls
import Qtui.Examples.DeviceCockpit

Rectangle {
    id: page
    color: "transparent"

    property var toasts: null
    readonly property var network: CockpitBackend.network
    readonly property var current: {
        const list = network.interfaces
        const index = nicList.currentIndex
        return index >= 0 && index < list.length ? list[index] : null
    }

    onVisibleChanged: {
        if (visible) {
            nicList.focus = true
            if (nicList.currentIndex < 0 && page.network.interfaces.length > 0)
                nicList.currentIndex = 0
        }
    }

    Keys.onPressed: (event) => {
        if (!page.network.canConfigure || !page.current) {
            return
        }
        if (event.key === Qt.Key_E) {
            editDialog.openFor(page.current.name, page.current.ipv4,
                               page.current.prefix, page.current.gateway)
            event.accepted = true
        } else if (event.key === Qt.Key_D) {
            dhcpDialog.title = "SWITCH TO DHCP"
            dhcpDialog.message = "Request a DHCP lease on " + page.current.name + "?"
            dhcpDialog.open()
            event.accepted = true
        }
    }

    Connections {
        target: page.network
        function onApplyFinished(name, ok, message) {
            if (page.toasts)
                page.toasts.showToast(ok ? "Network" : "Apply failed", message,
                                      ok ? "success" : "error")
        }
    }

    Row {
        anchors.fill: parent
        spacing: 1

        Panel {
            title: "INTERFACES"
            titleColor: Theme.textAccent
            width: Math.floor((parent.width - 1) / 2)
            height: parent.height
            color: Theme.bgPanel
            border.color: Theme.borderNormal

            ListView {
                id: nicList
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                model: page.network.interfaces
                highlightBackgroundColor: Theme.bgActiveTab

                delegate: Rectangle {
                    id: nicRow
                    required property var modelData
                    width: nicList.width - 3
                    height: 1
                    color: ListView.isCurrentItem ? Theme.bgActiveTab : "transparent"

                    Row {
                        spacing: 1
                        Text {
                            text: (nicRow.modelData.name + "       ").substring(0, 8)
                            color: Theme.textPrimary
                            bold: true
                        }
                        StatusBadge {
                            status: nicRow.modelData.up ? "success" : "error"
                            text: nicRow.modelData.up ? "UP" : "DOWN"
                            showIcon: false
                        }
                        Text {
                            text: nicRow.modelData.ipv4 !== ""
                                  ? nicRow.modelData.ipv4 : "no address"
                            color: nicRow.modelData.ipv4 !== ""
                                   ? Theme.textAccent : Theme.textDim
                        }
                    }
                }
            }
        }

        Panel {
            title: "DETAILS"
            titleColor: Theme.textAccent
            width: parent.width - Math.floor((parent.width - 1) / 2) - 1
            height: parent.height
            color: Theme.bgPanel
            border.color: Theme.borderNormal

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                spacing: 0
                visible: page.current !== null

                Row {
                    spacing: 1
                    Text { text: "Interface:"; color: Theme.textSecondary }
                    Text {
                        text: page.current ? page.current.name : ""
                        color: Theme.textPrimary
                        bold: true
                    }
                }
                Row {
                    spacing: 1
                    Text { text: "State:    "; color: Theme.textSecondary }
                    Text {
                        text: page.current ? (page.current.up ? "up" : "down") : ""
                        color: page.current && page.current.up
                               ? Theme.statusOk : Theme.textDanger
                    }
                }
                Row {
                    spacing: 1
                    Text { text: "Method:   "; color: Theme.textSecondary }
                    Text {
                        text: page.current ? page.current.method : ""
                        color: Theme.textPrimary
                    }
                }
                Row {
                    spacing: 1
                    Text { text: "IPv4:     "; color: Theme.textSecondary }
                    Text {
                        text: {
                            if (!page.current || page.current.ipv4 === "")
                                return "—"
                            let address = page.current.ipv4
                            if (page.current.prefix > 0)
                                address += "/" + page.current.prefix
                            return address
                        }
                        color: Theme.textAccent
                    }
                }
                Row {
                    spacing: 1
                    Text { text: "Gateway:  "; color: Theme.textSecondary }
                    Text {
                        text: page.current && page.current.gateway !== ""
                              ? page.current.gateway : "—"
                        color: Theme.textPrimary
                    }
                }
                Row {
                    spacing: 1
                    Text { text: "MAC:      "; color: Theme.textSecondary }
                    Text {
                        text: page.current && page.current.mac !== ""
                              ? page.current.mac : "—"
                        color: Theme.textDim
                    }
                }
                Text { text: " " }
                Text {
                    text: page.network.canConfigure
                          ? "[↑↓] Select  [E] Edit static IPv4  [D] Use DHCP"
                          : "[↑↓] Select  — configuration backend unavailable, read-only"
                    color: Theme.textDim
                }
            }
        }
    }

    EditIpv4Dialog {
        id: editDialog
        onApplyRequested: (address, prefix, gateway) => {
            page.network.applyIpv4(editDialog.interfaceName, "static",
                                   address, prefix, gateway)
        }
    }

    ConfirmDialog {
        id: dhcpDialog
        onAccepted: page.network.applyIpv4(page.current.name, "dhcp", "", 0, "")
    }
}
