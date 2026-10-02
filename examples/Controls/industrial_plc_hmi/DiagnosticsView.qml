import QtQml 2.15
import Terminal 1.0
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal
    padding: 1

    property int cycles: 0
    property bool valveA: false
    property bool valveB: false
    property string machineState: "idle"

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "🔌 PLC HARDWARE & SUBSYSTEM DIAGNOSTICS"
            color: HmiTheme.textHighlight
            bold: true
        }

        Row {
            width: parent ? parent.width : 78
            spacing: 2

            // Digital Inputs / Outputs Column
            Column {
                width: Math.floor((parent.width - 2) * 0.48)
                spacing: 1

                Text { text: "📥 DIGITAL INPUTS (DI)"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "DI-01 [E-STOP LOOP]    : [● NORMAL]"; color: HmiTheme.textSuccess }
                Text { text: "DI-02 [THERMAL OVERLOAD]: " + (root.machineState === "error" ? "[▲ TRIP! ]" : "[● NORMAL]"); color: root.machineState === "error" ? HmiTheme.textDanger : HmiTheme.textSuccess }
                Text { text: "DI-03 [PRESSURE SWITCH] : [● CLOSED]"; color: HmiTheme.textPrimary }
                Text { text: "DI-04 [FLOW SENSOR]     : [● ACTIVE]"; color: HmiTheme.textPrimary }

                Text { text: "📤 DIGITAL OUTPUTS (DO)"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "DO-01 [VALVE A COIL]   : " + (root.valveA ? "[● ON ]" : "[○ OFF]"); color: root.valveA ? HmiTheme.textSuccess : HmiTheme.textDim }
                Text { text: "DO-02 [VALVE B COIL]   : " + (root.valveB ? "[● ON ]" : "[○ OFF]"); color: root.valveB ? HmiTheme.textSuccess : HmiTheme.textDim }
                Text { text: "DO-03 [MAIN CONTACTOR] : " + (root.machineState === "processing" ? "[● ON ]" : "[○ OFF]"); color: root.machineState === "processing" ? HmiTheme.textSuccess : HmiTheme.textDim }
            }

            // Power & Communications Column
            Column {
                width: parent.width - Math.floor((parent.width - 2) * 0.48) - 2
                spacing: 1

                Text { text: "⚡ POWER & ANALOG BUS HEALTH"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "24V DC Field Power  : 24.1 VDC [OK]"; color: HmiTheme.textSuccess }
                Text { text: "4-20mA Sensor Loop  : 11.4 mA  [OK]"; color: HmiTheme.textSuccess }
                Text { text: "Internal CPU Temp   : 38.2 °C  [OK]"; color: HmiTheme.textSuccess }

                Text { text: "🌐 PROFINET / MODBUS BUS STACK"; color: HmiTheme.textSecondary; bold: true }
                Text { text: "Packets Transmitted : " + (root.cycles * 12); color: HmiTheme.textPrimary }
                Text { text: "Packets Received    : " + (root.cycles * 12); color: HmiTheme.textPrimary }
                Text { text: "CRC Error Counter   : 0 ERRORS"; color: HmiTheme.textSuccess }
            }
        }
    }
}
