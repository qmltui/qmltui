import QtQml
import Qtui
import IndustrialPlcHmi

Rectangle {
    id: root
    property int targetSpeed: 1500
    property double maxTempLimit: 85.0
    signal targetSpeedRequested(string input)
    signal tempLimitRequested(string input)

    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Text {
            text: "⚙️ SYSTEM PARAMETERS & SAFETY THRESHOLDS"
            color: HmiTheme.textHighlight
            bold: true
        }

        Row {
            width: parent.width
            spacing: 2

            Column {
                width: Math.floor((parent.width - 2) * 0.48)
                spacing: 0

                Text { text: "1. TARGET SPEED SETPOINT (RPM):"; color: HmiTheme.textPrimary }
                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: setRpmInput.activeFocus ? "double" : "single"
                    border.color: setRpmInput.activeFocus ? HmiTheme.borderFocus : HmiTheme.borderNormal

                    TextInput {
                        id: setRpmInput
                        anchors.fill: parent
                        anchors.margins: 1
                        text: root.targetSpeed.toString()
                        color: HmiTheme.textPrimary
                        onAccepted: {
                            root.targetSpeedRequested(setRpmInput.text)
                            setRpmInput.text = root.targetSpeed.toString()
                        }
                    }
                }

                Text { text: "2. OVER-TEMP SAFETY LIMIT (°C):"; color: HmiTheme.textPrimary }
                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: setTempInput.activeFocus ? "double" : "single"
                    border.color: setTempInput.activeFocus ? HmiTheme.borderFocus : HmiTheme.borderNormal

                    TextInput {
                        id: setTempInput
                        anchors.fill: parent
                        anchors.margins: 1
                        text: root.maxTempLimit.toString()
                        color: HmiTheme.textPrimary
                        onAccepted: {
                            root.tempLimitRequested(setTempInput.text)
                            setTempInput.text = root.maxTempLimit.toString()
                        }
                    }
                }
            }

            Column {
                width: parent.width - Math.floor((parent.width - 2) * 0.48) - 2
                spacing: 0

                Text { text: "📋 SYSTEM DEFAULT CALIBRATION"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "Max Speed Safety Ceiling : 3000 RPM"; color: HmiTheme.textDim }
                Text { text: "Max Temp Trip Ceiling    : 110.0 °C"; color: HmiTheme.textDim }
                Text { text: "Telemetry Sample Rate    : 1000 ms"; color: HmiTheme.textDim }
                Text { text: "Interlock Safety Delay   : 500 ms"; color: HmiTheme.textDim }
                Text { text: ""; height: 1 }
                Text { text: "Press [Enter] after typing setpoint to apply."; color: HmiTheme.textWarning }
            }
        }
    }
}
