pragma ComponentBehavior: Bound

import QtQml 2.15
import Qtui
import DinGatewayApp

Rectangle {
    id: trafficRoot
    color: "transparent"

    property var packetLogs: GatewayBackend.packetLogs
    property int totalTx: GatewayBackend.totalTx
    property int totalRx: GatewayBackend.totalRx
    property int crcErrors: GatewayBackend.crcErrors
    property int packetsPerSec: GatewayBackend.packetsPerSec
    property real busLoadPct: GatewayBackend.busLoadPct

    Column {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 1

        // TOP METRICS BAR
        Rectangle {
            width: parent.width
            height: 3
            border.style: "single"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgPanel

            Row {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 2

                Text { text: "TX: " + trafficRoot.totalTx; color: GatewayTheme.textSuccess; bold: true }
                Text { text: "RX: " + trafficRoot.totalRx; color: GatewayTheme.textAccent; bold: true }
                Text { text: "CRC Err: " + trafficRoot.crcErrors; color: trafficRoot.crcErrors > 0 ? GatewayTheme.textWarning : GatewayTheme.textPrimary; bold: true }
                Text { text: "Rate: " + trafficRoot.packetsPerSec + " p/s"; color: GatewayTheme.textPrimary }
                Text { text: "Bus Load: " + trafficRoot.busLoadPct.toFixed(1) + "% " + GatewayTheme.makeProgressBar(trafficRoot.busLoadPct, 0, 100, 10, "segmented"); color: GatewayTheme.textAccent; bold: true }
            }
        }

        // PACKET LOGS TABLE HEADER
        Rectangle {
            width: parent.width
            height: 1
            color: GatewayTheme.bgActiveTab

            Row {
                anchors.fill: parent
                spacing: 1

                Text { width: 14; text: "TIME"; color: GatewayTheme.textPrimary; bold: true }
                Text { width: 10; text: "BUS"; color: GatewayTheme.textPrimary; bold: true }
                Text { width: 12; text: "ADDR/ID"; color: GatewayTheme.textPrimary; bold: true }
                Text { width: 16; text: "FUNCTION"; color: GatewayTheme.textPrimary; bold: true }
                Text { width: 30; text: "PAYLOAD"; color: GatewayTheme.textPrimary; bold: true }
                Text { width: 16; text: "STATUS"; color: GatewayTheme.textPrimary; bold: true }
            }
        }

        // PACKET LOGS REPEATER LIST
        Repeater {
            model: trafficRoot.packetLogs.length

            Rectangle {
                id: packetEntry
                required property int index
                width: trafficRoot.width - 2
                height: 1
                color: packetEntry.index % 2 === 0 ? "transparent" : GatewayTheme.bgPanel

                readonly property var itemData: trafficRoot.packetLogs[packetEntry.index]

                Row {
                    anchors.fill: parent
                    spacing: 1

                    Text { width: 14; text: packetEntry.itemData.time; color: GatewayTheme.textSecondary }
                    Text { width: 10; text: packetEntry.itemData.bus; color: packetEntry.itemData.bus.indexOf("CAN") >= 0 ? GatewayTheme.textAccent : GatewayTheme.textWarning; bold: true }
                    Text { width: 12; text: packetEntry.itemData.addr; color: GatewayTheme.textPrimary }
                    Text { width: 16; text: packetEntry.itemData.func; color: GatewayTheme.textPrimary }
                    Text { width: 30; text: packetEntry.itemData.payload; color: GatewayTheme.textPrimary }
                    Text { width: 16; text: packetEntry.itemData.status; color: packetEntry.itemData.status.indexOf("ERR") >= 0 ? GatewayTheme.textDanger : GatewayTheme.textSuccess; bold: packetEntry.itemData.status.indexOf("ERR") >= 0 }
                }
            }
        }
    }
}
