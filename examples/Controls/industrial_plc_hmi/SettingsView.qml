import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal
    padding: 1

    property int targetSpeed: 1500
    property double maxTempLimit: 85.0
    signal speedChanged(int newSpeed)
    signal tempLimitChanged(double newLimit)

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "⚙️ SYSTEM PARAMETERS & SAFETY THRESHOLDS"
            color: HmiTheme.textHighlight
            bold: true
        }

        Row {
            width: parent ? parent.width : 78
            spacing: 2

            Column {
                width: Math.floor((parent.width - 2) * 0.48)
                spacing: 1

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
                            let val = parseInt(setRpmInput.text);
                            if (!isNaN(val) && val >= 0 && val <= 3000) {
                                root.speedChanged(val);
                            }
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
                            let val = parseFloat(setTempInput.text);
                            if (!isNaN(val) && val >= 30.0 && val <= 110.0) {
                                root.tempLimitChanged(val);
                            }
                        }
                    }
                }
            }

            Column {
                width: parent.width - Math.floor((parent.width - 2) * 0.48) - 2
                spacing: 1

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
