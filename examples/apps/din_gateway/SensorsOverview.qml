import QtQml 2.15
import Qtui
import Qtui.Controls
import Qtui.Extras
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
                    Text { text: (sensorValues[0] ? sensorValues[0].toFixed(1) : "0.0") + "°C "; color: GatewayTheme.textSuccess; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[0] || 0, 20, 80, 10, "segmented"); color: GatewayTheme.textSuccess }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[0] || [0]
                    minVal: 20
                    maxVal: 80
                    color: GatewayTheme.textSuccess
                }

                // AI-02
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-02 Volt: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorValues[1] ? sensorValues[1].toFixed(1) : "0.0") + "V  "; color: GatewayTheme.textAccent; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[1] || 0, 40, 60, 10, "segmented"); color: GatewayTheme.textAccent }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[1] || [0]
                    minVal: 40
                    maxVal: 60
                    color: GatewayTheme.textAccent
                }

                // AI-03
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-03 Curr: "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorValues[2] ? sensorValues[2].toFixed(1) : "0.0") + "A  "; color: GatewayTheme.textWarning; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[2] || 0, 0, 30, 10, "segmented"); color: GatewayTheme.textWarning }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[2] || [0]
                    minVal: 0
                    maxVal: 30
                    color: GatewayTheme.textWarning
                }

                // AI-04
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-04 Power:"; color: GatewayTheme.textPrimary }
                    Text { text: (sensorValues[3] ? sensorValues[3].toFixed(2) : "0.00") + "kW "; color: GatewayTheme.textPrimary; bold: true }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[3] || 0, 0, 5, 10, "segmented"); color: GatewayTheme.textPrimary }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[3] || [0]
                    minVal: 0
                    maxVal: 5
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
                    Text { text: (sensorValues[4] ? sensorValues[4].toFixed(1) : "0.0") + "%  "; color: GatewayTheme.textSecondary }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[4] || 0, 0, 100, 10, "segmented"); color: GatewayTheme.textSecondary }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[4] || [0]
                    minVal: 0
                    maxVal: 100
                    color: GatewayTheme.textSecondary
                }

                // AI-06
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-06 Flow:  "; color: GatewayTheme.textPrimary }
                    Text { text: (sensorValues[5] ? sensorValues[5].toFixed(1) : "0.0") + "L/m "; color: GatewayTheme.textAccent }
                    Text { text: GatewayTheme.makeProgressBar(sensorValues[5] || 0, 0, 30, 10, "segmented"); color: GatewayTheme.textAccent }
                }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: sensorHistories[5] || [0]
                    minVal: 0
                    maxVal: 30
                    color: GatewayTheme.textAccent
                }

                // AI-07 & AI-08
                Row {
                    width: parent.width
                    spacing: 1
                    Text { text: "AI-07 Vib:  " + (sensorValues[6] ? sensorValues[6].toFixed(2) : "0.00") + "mm/s  "; color: GatewayTheme.textSuccess }
                    Text { text: "AI-08 Pres: " + (sensorValues[7] ? sensorValues[7].toFixed(1) : "0.0") + "kPa"; color: GatewayTheme.textPrimary }
                }

                Text { text: "🔌 RELAY DIGITAL OUTPUTS (RO-01 .. RO-04)"; color: GatewayTheme.textAccent; bold: true }

                Row {
                    width: parent.width
                    spacing: 1

                    Text { text: "RO-1 [Fan]: " + (relay1 ? "[🟢 ON ]" : "[⚪ OFF]"); color: relay1 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: relay1 }
                    Text { text: " RO-2 [Heat]: " + (relay2 ? "[🟢 ON ]" : "[⚪ OFF]"); color: relay2 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: relay2 }
                }
                Row {
                    width: parent.width
                    spacing: 1

                    Text { text: "RO-3 [Srn]: " + (relay3 ? "[🟡 ARM]" : "[⚪ OFF]"); color: relay3 ? GatewayTheme.textWarning : GatewayTheme.textDim; bold: relay3 }
                    Text { text: " RO-4 [Aux ]: " + (relay4 ? "[🟢 ON ]" : "[⚪ OFF]"); color: relay4 ? GatewayTheme.textSuccess : GatewayTheme.textDim; bold: relay4 }
                }
            }
        }
    }
}
