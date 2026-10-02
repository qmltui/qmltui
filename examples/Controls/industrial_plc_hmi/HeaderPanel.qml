import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    height: 3
    property string machineState: "idle"
    property string stationId: "STN#42"

    border.style: "double"
    border.color: root.machineState === "error" 
                  ? HmiTheme.borderError 
                  : (root.machineState === "maintenance" ? HmiTheme.textWarning : (root.machineState === "processing" ? HmiTheme.textHighlight : HmiTheme.statusIdle))
    color: root.machineState === "error" ? HmiTheme.headerBgError : HmiTheme.headerBg

    Text {
        x: 2
        y: 1
        text: "🏭 PLC CONTROLLER & HMI INTERFACE - " + root.stationId
        bold: true
        color: HmiTheme.textPrimary
    }

    // State Badge pinned to top-right corner
    Rectangle {
        width: 16
        height: 1
        anchors.right: parent.right
        anchors.rightMargin: 2
        y: 1
        color: root.machineState === "error" 
               ? HmiTheme.statusError 
               : (root.machineState === "maintenance" ? HmiTheme.statusMaintenance : (root.machineState === "processing" ? HmiTheme.statusProcessing : HmiTheme.statusIdle))

        Text {
            anchors.centerIn: parent
            text: root.machineState === "error"
                  ? "[▲ FAULT!]"
                  : (root.machineState === "maintenance" 
                     ? "[⚙ MAINT]" 
                     : (root.machineState === "processing" ? "[● RUNNING]" : "[■ STANDBY]"))
            color: HmiTheme.textPrimary
            bold: true
        }
    }
}
