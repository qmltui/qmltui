import QtQml
import Qamui
import Qamui.Controls
import IndustrialPlcHmi

Rectangle {
    id: root
    property int cycles: 0
    property bool valveA: false
    property bool valveB: false
    property int machineState: PlcTypes.Idle

    readonly property list<real> busVoltageHistory: [24.1, 24.1, 24.0, 24.2, 24.1, 24.1, 24.0, 24.2, 24.1, 24.1, 24.0, 24.1, 24.2, 24.1, 24.1]
    readonly property list<real> sensorLoopHistory: [11.4, 11.5, 11.4, 11.3, 11.5, 11.4, 11.6, 11.4, 11.5, 11.4, 11.4, 11.5, 11.4, 11.3, 11.5]

    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Text {
            text: "🔌 PLC HARDWARE & SUBSYSTEM DIAGNOSTICS"
            color: HmiTheme.textHighlight
            bold: true
        }

        Row {
            width: parent.width
            spacing: 2

            // Digital Inputs / Outputs Column
            Column {
                width: Math.floor((parent.width - 2) * 0.48)
                spacing: 0

                Text { text: "📥 DIGITAL INPUTS (DI)"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "DI-01 [E-STOP LOOP]    : [● NORMAL]"; color: HmiTheme.textSuccess }
                Text { text: "DI-02 [THERMAL OVERLOAD]: " + (root.machineState === PlcTypes.Error ? "[▲ TRIP! ]" : "[● NORMAL]"); color: root.machineState === PlcTypes.Error ? HmiTheme.textDanger : HmiTheme.textSuccess }
                Text { text: "DI-03 [PRESSURE SWITCH] : [● CLOSED]"; color: HmiTheme.textPrimary }
                Text { text: "DI-04 [FLOW SENSOR]     : [● ACTIVE]"; color: HmiTheme.textPrimary }

                Text { text: "📤 DIGITAL OUTPUTS (DO)"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "DO-01 [VALVE A COIL]   : " + (root.valveA ? "[● ON ]" : "[○ OFF]"); color: root.valveA ? HmiTheme.textSuccess : HmiTheme.textDim }
                Text { text: "DO-02 [VALVE B COIL]   : " + (root.valveB ? "[● ON ]" : "[○ OFF]"); color: root.valveB ? HmiTheme.textSuccess : HmiTheme.textDim }
                Text { text: "DO-03 [MAIN CONTACTOR] : " + (root.machineState === PlcTypes.Processing ? "[● ON ]" : "[○ OFF]"); color: root.machineState === PlcTypes.Processing ? HmiTheme.textSuccess : HmiTheme.textDim }
            }

            // Power & Communications Column
            Column {
                width: parent.width - Math.floor((parent.width - 2) * 0.48) - 2
                spacing: 0

                Text { text: "⚡ POWER & ANALOG BUS TELEMETRY"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "24V DC Field Power  : 24.1 VDC [OK]"; color: HmiTheme.textSuccess }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: root.busVoltageHistory
                    from: 23.0
                    to: 25.0
                    color: HmiTheme.statusIdle
                }

                Text { text: "4-20mA Sensor Loop  : 11.4 mA  [OK]"; color: HmiTheme.textSuccess }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: root.sensorLoopHistory
                    from: 4.0
                    to: 20.0
                    color: HmiTheme.textHighlight
                }

                Text { text: "🌐 PROFINET / MODBUS BUS STACK"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "Packets Transmitted : " + (root.cycles * 12); color: HmiTheme.textPrimary }
                Sparkline {
                    width: parent.width
                    height: 1
                    values: [5, 12, 18, 25, 40, 65, 80, 95, 70, 85, 100, 110, 120, 130, 145]
                    color: HmiTheme.textAccent
                }
                Text { text: "CRC Error Counter   : 0 ERRORS"; color: HmiTheme.textSuccess }
            }
        }
    }
}
