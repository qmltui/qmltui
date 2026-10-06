import QtQml
import Qtui
import Qtui.Controls
import DinGatewayApp

Rectangle {
    id: navRoot
    height: 3
    color: GatewayTheme.bgPanel
    border.style: "single"
    border.color: GatewayTheme.borderNormal

    property int currentTab: 0 // 0: Sensors, 1: Bus Traffic, 2: System Network, 3: Config

    signal tabSelected(int index)

    Row {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Button {
            width: Math.max(18, Math.floor((parent.width - 3) / 4))
            height: 1
            text: navRoot.currentTab === 0 ? "▶ [F1] 📊 Sensors (8)" : "  [F1] 📊 Sensors (8)"
            normalColor: navRoot.currentTab === 0 ? GatewayTheme.bgActiveTab : "transparent"
            focusColor: GatewayTheme.borderFocus
            textColor: navRoot.currentTab === 0 ? GatewayTheme.textPrimary : GatewayTheme.textSecondary
            onClicked: navRoot.tabSelected(0)
        }

        Button {
            width: Math.max(20, Math.floor((parent.width - 3) / 4))
            height: 1
            text: navRoot.currentTab === 1 ? "▶ [F2] 📡 Modbus/CAN" : "  [F2] 📡 Modbus/CAN"
            normalColor: navRoot.currentTab === 1 ? GatewayTheme.bgActiveTab : "transparent"
            focusColor: GatewayTheme.borderFocus
            textColor: navRoot.currentTab === 1 ? GatewayTheme.textPrimary : GatewayTheme.textSecondary
            onClicked: navRoot.tabSelected(1)
        }

        Button {
            width: Math.max(20, Math.floor((parent.width - 3) / 4))
            height: 1
            text: navRoot.currentTab === 2 ? "▶ [F3] 🌐 System/Net" : "  [F3] 🌐 System/Net"
            normalColor: navRoot.currentTab === 2 ? GatewayTheme.bgActiveTab : "transparent"
            focusColor: GatewayTheme.borderFocus
            textColor: navRoot.currentTab === 2 ? GatewayTheme.textPrimary : GatewayTheme.textSecondary
            onClicked: navRoot.tabSelected(2)
        }

        Button {
            width: Math.max(18, Math.floor((parent.width - 3) / 4))
            height: 1
            text: navRoot.currentTab === 3 ? "▶ [F4] ⚙ Config" : "  [F4] ⚙ Config"
            normalColor: navRoot.currentTab === 3 ? GatewayTheme.bgActiveTab : "transparent"
            focusColor: GatewayTheme.borderFocus
            textColor: navRoot.currentTab === 3 ? GatewayTheme.textPrimary : GatewayTheme.textSecondary
            onClicked: navRoot.tabSelected(3)
        }
    }
}
