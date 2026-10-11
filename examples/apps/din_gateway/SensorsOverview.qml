import QtQml
import Qamui
import Qamui.Controls
import DinGatewayApp

Rectangle {
    id: sensorsRoot
    color: "transparent"

    property var sensorValues: GatewayBackend.sensorValues
    property var sensorHistories: GatewayBackend.sensorHistories
    property bool relay1: GatewayBackend.relay1
    property bool relay2: GatewayBackend.relay2
    property bool relay3: GatewayBackend.relay3
    property bool relay4: GatewayBackend.relay4

    Row {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 1

        // LEFT COLUMN: CHANNELS 1 - 4
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

                Text { text: "📊 ANALOG CHANNELS (AI-01 .. AI-04)"; color: GatewayTheme.textAccent; bold: true }

                // AI-01
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-01 Temp: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[0] ? sensorsRoot.sensorValues[0].toFixed(1) : "0.0") + "°C "; color: GatewayTheme.textSuccess; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[0] || 0, 20, 80, 10, "segmented"); color: GatewayTheme.textSuccess }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[0] || [0]
                    from: 20
                    to: 80
                    color: GatewayTheme.textSuccess
                }

                // AI-02
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-02 Volt: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[1] ? sensorsRoot.sensorValues[1].toFixed(1) : "0.0") + "V  "; color: GatewayTheme.textAccent; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[1] || 0, 40, 60, 10, "segmented"); color: GatewayTheme.textAccent }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[1] || [0]
                    from: 40
                    to: 60
                    color: GatewayTheme.textAccent
                }

                // AI-03
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-03 Curr: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[2] ? sensorsRoot.sensorValues[2].toFixed(1) : "0.0") + "A  "; color: GatewayTheme.textWarning; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[2] || 0, 0, 30, 10, "segmented"); color: GatewayTheme.textWarning }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[2] || [0]
                    from: 0
                    to: 30
                    color: GatewayTheme.textWarning
                }

                // AI-04
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-04 Power:"; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[3] ? sensorsRoot.sensorValues[3].toFixed(2) : "0.00") + "kW "; color: GatewayTheme.textPrimary; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[3] || 0, 0, 5, 10, "segmented"); color: GatewayTheme.textPrimary }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[3] || [0]
                    from: 0
                    to: 5
                    color: GatewayTheme.textPrimary
                }
            }
        }

        // RIGHT COLUMN: CHANNELS 5 - 8 & RELAYS
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

                Text { text: "📊 ANALOG CHANNELS (AI-05 .. AI-08)"; color: GatewayTheme.textAccent; bold: true }

                // AI-05
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-05 Humid: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[4] ? sensorsRoot.sensorValues[4].toFixed(1) : "0.0") + "%  "; color: GatewayTheme.textSecondary }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[4] || 0, 0, 100, 10, "segmented"); color: GatewayTheme.textSecondary }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[4] || [0]
                    from: 0
                    to: 100
                    color: GatewayTheme.textSecondary
                }

                // AI-06
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-06 Flow:  "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorsRoot.sensorValues[5] ? sensorsRoot.sensorValues[5].toFixed(1) : "0.0") + "L/m "; color: GatewayTheme.textAccent }
                    Text { text: GatewayTheme.makeProgressBar(sensorsRoot.sensorValues[5] || 0, 0, 30, 10, "segmented"); color: GatewayTheme.textAccent }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorsRoot.sensorHistories[5] || [0]
                    from: 0
                    to: 30
                    color: GatewayTheme.textAccent
                }

                // AI-07 & AI-08
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-07 Vib:  " + (sensorsRoot.sensorValues[6] ? sensorsRoot.sensorValues[6].toFixed(2) : "0.00") + "mm/s  "; color: GatewayTheme.textSuccess }
                    Text { text: "AI-08 Pres: " + (sensorsRoot.sensorValues[7] ? sensorsRoot.sensorValues[7].toFixed(1) : "0.0") + "kPa"; color: GatewayTheme.textPrimary }
                }

                Text { text: "🔌 RELAY DIGITAL OUTPUTS (RO-01 .. RO-04)"; color: GatewayTheme.textAccent; bold: true }

                Row {
                    width: parent.width
                    spacing: 1

                    Text { text: "RO-1 [Fan]: " + (sensorsRoot.relay1 ? "[🟢 ON ]" : "[⚪ OFF]"); color: sensorsRoot.relay1 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: sensorsRoot.relay1 }
                    Text { text: " RO-2 [Heat]: " + (sensorsRoot.relay2 ? "[🟢 ON ]" : "[⚪ OFF]"); color: sensorsRoot.relay2 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: sensorsRoot.relay2 }
                }
                Row {
                    width: parent.width
                    spacing: 1

                    Text { text: "RO-3 [Srn]: " + (sensorsRoot.relay3 ? "[🟡 ARM]" : "[⚪ OFF]"); color: sensorsRoot.relay3 ? GatewayTheme.textWarning : GatewayTheme.textDim; bold: sensorsRoot.relay3 }
                    Text { text: " RO-4 [Aux ]: " + (sensorsRoot.relay4 ? "[🟢 ON ]" : "[⚪ OFF]"); color: sensorsRoot.relay4 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: sensorsRoot.relay4 }
                }
            }
        }
    }
}
