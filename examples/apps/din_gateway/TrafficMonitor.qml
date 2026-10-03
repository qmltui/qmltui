import QtQml 2.15
import Qtui
import Qtui.Controls
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

                Text { text: "TX: " + totalTx; color: GatewayTheme.textSuccess; bold: true }
                Text { text: "RX: " + totalRx; color: GatewayTheme.textAccent; bold: true }
                Text { text: "CRC Err: " + crcErrors; color: crcErrors > 0 ? GatewayTheme.textWarning : GatewayTheme.textPrimary; bold: true }
                Text { text: "Rate: " + packetsPerSec + " p/s"; color: GatewayTheme.textPrimary }
                Text { text: "Bus Load: " + busLoadPct.toFixed(1) + "% " + GatewayTheme.makeProgressBar(busLoadPct, 0, 100, 10, "segmented"); color: GatewayTheme.textAccent; bold: true }
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
            model: packetLogs.length

            Rectangle {
                width: trafficRoot.width - 2
                height: 1
                color: index % 2 === 0 ? "transparent" : GatewayTheme.bgPanel

                property var itemData: packetLogs[index]

                Row {
                    anchors.fill: parent
                    spacing: 1

                    Text { width: 14; text: itemData.time; color: GatewayTheme.textSecondary }
                    Text { width: 10; text: itemData.bus; color: itemData.bus.indexOf("CAN") >= 0 ? GatewayTheme.textAccent : GatewayTheme.textWarning; bold: true }
                    Text { width: 12; text: itemData.addr; color: GatewayTheme.textPrimary }
                    Text { width: 16; text: itemData.func; color: GatewayTheme.textPrimary }
                    Text { width: 30; text: itemData.payload; color: GatewayTheme.textPrimary }
                    Text { width: 16; text: itemData.status; color: itemData.status.indexOf("ERR") >= 0 ? GatewayTheme.textDanger : GatewayTheme.textSuccess; bold: itemData.status.indexOf("ERR") >= 0 }
                }
            }
        }
    }
}
