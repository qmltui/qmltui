import QtQml 2.15
import Terminal 1.0
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    height: 1
    property int currentTab: 0 // 0: Dashboard, 1: Alarms, 2: Diagnostics, 3: Settings
    property int alarmCount: 0

    color: "#181a1f"

    Row {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === 0 ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === 0 ? "▶ [F1] DASHBOARD" : "  [F1] DASHBOARD"
                color: root.currentTab === 0 ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === 0
            }
        }

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === 1 ? HmiTheme.borderActive : (root.alarmCount > 0 ? "#4b1818" : "#282c34")
            Text {
                anchors.centerIn: parent
                text: (root.currentTab === 1 ? "▶ [F2] ALARMS (" : "  [F2] ALARMS (") + root.alarmCount + ")"
                color: root.currentTab === 1 ? "black" : (root.alarmCount > 0 ? HmiTheme.textDanger : HmiTheme.textSecondary)
                bold: root.currentTab === 1 || root.alarmCount > 0
            }
        }

        Rectangle {
            width: Math.floor(parent.width / 4)
            height: 1
            color: root.currentTab === 2 ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === 2 ? "▶ [F3] DIAGNOSTICS" : "  [F3] DIAGNOSTICS"
                color: root.currentTab === 2 ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === 2
            }
        }

        Rectangle {
            width: parent.width - Math.floor(parent.width / 4) * 3
            height: 1
            color: root.currentTab === 3 ? HmiTheme.borderActive : "#282c34"
            Text {
                anchors.centerIn: parent
                text: root.currentTab === 3 ? "▶ [F4] SETTINGS" : "  [F4] SETTINGS"
                color: root.currentTab === 3 ? "black" : HmiTheme.textSecondary
                bold: root.currentTab === 3
            }
        }
    }
}
