import QtQml
import Qtui
import IndustrialPlcHmi

Rectangle {
    id: root
    property int machineState: PlcTypes.Idle
    property string stationId: "STN#42"
    readonly property bool validState: root.machineState === PlcTypes.Idle
                                       || root.machineState === PlcTypes.Processing
                                       || root.machineState === PlcTypes.Error
                                       || root.machineState === PlcTypes.Maintenance

    height: 3

    border.style: "double"
    border.color: root.machineState === PlcTypes.Error || !root.validState
                  ? HmiTheme.borderError 
                  : (root.machineState === PlcTypes.Maintenance
                     ? HmiTheme.textWarning
                     : (root.machineState === PlcTypes.Processing
                        ? HmiTheme.textHighlight : HmiTheme.statusIdle))
    color: root.machineState === PlcTypes.Error || !root.validState
           ? HmiTheme.headerBgError : HmiTheme.headerBg

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
        color: root.machineState === PlcTypes.Error || !root.validState
               ? HmiTheme.statusError 
               : (root.machineState === PlcTypes.Maintenance
                  ? HmiTheme.statusMaintenance
                  : (root.machineState === PlcTypes.Processing
                     ? HmiTheme.statusProcessing : HmiTheme.statusIdle))

        Text {
            anchors.centerIn: parent
            text: !root.validState
                  ? "[? INVALID]"
                  : root.machineState === PlcTypes.Error
                  ? "[▲ FAULT!]"
                  : (root.machineState === PlcTypes.Maintenance
                     ? "[⚙ MAINT]" 
                     : (root.machineState === PlcTypes.Processing
                        ? "[● RUNNING]" : "[■ STANDBY]"))
            color: HmiTheme.textPrimary
            bold: true
        }
    }
}
