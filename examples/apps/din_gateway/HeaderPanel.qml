import QtQml 2.15
import Qtui
import Qtui.Controls
import DinGatewayApp

Rectangle {
    id: headerRoot
    height: 3
    color: GatewayTheme.bgHeader
    border.style: "double"
    border.color: GatewayTheme.borderNormal

    property string deviceId: "DIN-GW-9402"
    property string activeMode: "ONLINE"
    property bool mqttConnected: true
    property int uptimeSeconds: GatewayBackend.uptimeSeconds

    function formatUptime(secs) {
        let hrs = Math.floor(secs / 3600);
        let mins = Math.floor((secs % 3600) / 60);
        let s = secs % 60;
        return (hrs < 10 ? "0" + hrs : hrs) + ":" + (mins < 10 ? "0" + mins : mins) + ":" + (s < 10 ? "0" + s : s);
    }

    // LEFT ALIGNED STATUS ITEMS
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Text {
            text: "⚡ " + deviceId
            color: GatewayTheme.textAccent
            bold: true
        }

        Text {
            text: "SYS: ● " + activeMode
            color: activeMode === "ONLINE" ? GatewayTheme.textSuccess : GatewayTheme.textWarning
            bold: true
        }
    }

    // RIGHT ALIGNED STATUS ITEMS WITH SINGLE SUBTLE SPINNER
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Spinner {
            style: Spinner.Style.Dots
            color: GatewayTheme.textSuccess
            interval: 120
            running: mqttConnected
        }

        Text {
            text: "MQTT: " + (mqttConnected ? "CONNECTED" : "DISCONNECTED")
            color: mqttConnected ? GatewayTheme.textSuccess : GatewayTheme.textDanger
            bold: true
        }

        Text {
            text: "UP: " + formatUptime(uptimeSeconds)
            color: GatewayTheme.textSecondary
        }
    }
}
