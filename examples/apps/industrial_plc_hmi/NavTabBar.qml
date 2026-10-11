import QtQml
import Qamui
import IndustrialPlcHmi

Rectangle {
    id: root
    property int currentTab: PlcTypes.Dashboard
    property int alarmCount: 0

    height: 1
    color: "#181a1f"

    Row {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === PlcTypes.Dashboard ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === PlcTypes.Dashboard ? "▶ [F1] DASHBOARD" : "  [F1] DASHBOARD"
                color: root.currentTab === PlcTypes.Dashboard ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === PlcTypes.Dashboard
            }
        }

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === PlcTypes.Alarms ? HmiTheme.borderActive : (root.alarmCount > 0 ? "#4b1818" : "#282c34")
            Text {
                anchors.centerIn: parent
                text: (root.currentTab === PlcTypes.Alarms ? "▶ [F2] ALARMS (" : "  [F2] ALARMS (") + root.alarmCount + ")"
                color: root.currentTab === PlcTypes.Alarms ? "black" : (root.alarmCount > 0 ? HmiTheme.textDanger : HmiTheme.textSecondary)
                bold: root.currentTab === PlcTypes.Alarms || root.alarmCount > 0
            }
        }

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === PlcTypes.Diagnostics ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === PlcTypes.Diagnostics ? "▶ [F3] DIAGNOSTICS" : "  [F3] DIAGNOSTICS"
                color: root.currentTab === PlcTypes.Diagnostics ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === PlcTypes.Diagnostics
            }
        }

        Rectangle {
            width: parent.width - Math.floor(parent.width / 4) * 3
            height: 1
            color: root.currentTab === PlcTypes.Settings ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === PlcTypes.Settings ? "▶ [F4] SETTINGS" : "  [F4] SETTINGS"
                color: root.currentTab === PlcTypes.Settings ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === PlcTypes.Settings
            }
        }
    }
}
