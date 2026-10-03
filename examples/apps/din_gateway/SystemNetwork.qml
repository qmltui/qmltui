import QtQml 2.15
import Qtui
import Qtui.Controls
import Qtui.Extras
import DinGatewayApp

Rectangle {
    id: sysNetRoot
    color: "transparent"

    property string ethIp: "192.168.1.22"
    property string ethSubnet: "255.255.255.0"
    property string ethGateway: "192.168.1.1"
    property string ethMac: "B8:27:EB:A1:D8:8D"

    property string cellIp: "10.142.88.19"
    property string cellOperator: "Vodafone LTE"
    property int cellRssi: -68 // dBm
    property int cellRsrp: -92 // dBm

    property real cpuLoadPct: GatewayBackend.cpuLoadPct
    property real cpuTemp: GatewayBackend.cpuTemp
    property int memUsedMb: GatewayBackend.memUsedMb
    property int memTotalMb: 512
    property int diskUsedMb: 1280
    property int diskTotalMb: 7680

    Row {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 1

        // LEFT PANEL: NETWORK INTERFACES
        Rectangle {
            width: Math.floor((parent.width - 1) / 2)
            height: parent.height
            border.style: "single"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgPanel

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                Text { text: "🌐 ETHERNET (eth0)"; color: GatewayTheme.textAccent; bold: true }
                Text { text: "Link:   CONNECTED [100M Full]"; color: GatewayTheme.textSuccess }
                Text { text: "IP:     " + ethIp; color: GatewayTheme.textPrimary; bold: true }
                Text { text: "Subnet: " + ethSubnet; color: GatewayTheme.textSecondary }
                Text { text: "GW:     " + ethGateway; color: GatewayTheme.textSecondary }
                Text { text: "MAC:    " + ethMac; color: GatewayTheme.textDim }

                Text { text: "📱 CELLULAR 4G/LTE (wwan0)"; color: GatewayTheme.textAccent; bold: true }
                Text { text: "Net:    " + cellOperator; color: GatewayTheme.textPrimary }
                Text { text: "IP:     " + cellIp; color: GatewayTheme.textPrimary }
                Text { text: "RSSI:   " + cellRssi + "dBm " + GatewayTheme.makeProgressBar(100 + cellRssi, 0, 80, 8, "segmented"); color: GatewayTheme.textSuccess; bold: true }
                Text { text: "RSRP:   " + cellRsrp + " dBm"; color: GatewayTheme.textSecondary }
            }
        }

        // RIGHT PANEL: CPU, MEMORY, STORAGE & CLOUD MQTT
        Rectangle {
            width: parent.width - Math.floor((parent.width - 1) / 2) - 1
            height: parent.height
            border.style: "single"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgPanel

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                Text { text: "💻 HARDWARE & CPU"; color: GatewayTheme.textAccent; bold: true }
                Text { text: "SoC:    ARMv7-A / ARM64 MPU"; color: GatewayTheme.textPrimary }
                Text { text: "CPU:    " + cpuLoadPct.toFixed(1) + "% " + GatewayTheme.makeProgressBar(cpuLoadPct, 0, 100, 10, "segmented") + " (" + cpuTemp.toFixed(1) + "°C)"; color: GatewayTheme.textSuccess; bold: true }
                Text { text: "RAM:    " + memUsedMb + "MB/512MB " + GatewayTheme.makeProgressBar(memUsedMb, 0, 512, 10, "segmented"); color: GatewayTheme.textSecondary }
                Text { text: "Disk:   " + (diskUsedMb/1024).toFixed(1) + "GB/7.5GB " + GatewayTheme.makeProgressBar(diskUsedMb, 0, 7680, 10, "segmented"); color: GatewayTheme.textSecondary }

                Text { text: "☁ CLOUD MQTT SYNC"; color: GatewayTheme.textAccent; bold: true }
                Text { text: "Broker: mqtt.iot-cloud-gateway.com"; color: GatewayTheme.textPrimary }
                Text { text: "Topic:  telemetry/v1/din-gw-9402"; color: GatewayTheme.textDim }
                Text { text: "Ping:   18 ms (Status: OK ●)"; color: GatewayTheme.textSuccess }
                Text { text: "Queue:  0 pending msgs"; color: GatewayTheme.textSecondary }
            }
        }
    }
}
