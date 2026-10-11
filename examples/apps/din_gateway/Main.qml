import QtQml
import Qamui
import Qamui.Controls
import DinGatewayApp

Terminal {
    id: rootWindow
    color: "black"

    property int activeTab: 0 // 0: Sensors, 1: Bus Traffic, 2: Network/System, 3: Config

    // Mandatory Quit key handler + Tab & Config Key Handling
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return;
        }

        // F1-F4 always switch tabs
        if (event.key === Qt.Key_F1) {
            activeTab = 0;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F2) {
            activeTab = 1;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F3) {
            activeTab = 2;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F4) {
            activeTab = 3;
            event.accepted = true;
            return;
        }

        // If on Config tab (tab 3), forward key events to ConfigView first
        if (activeTab === 3) {
            if (configView.handleKey(event.key)) {
                event.accepted = true;
                return;
            }
        }

        // Default number key tab switching when not handled by active tab
        if (event.key === Qt.Key_1) {
            activeTab = 0;
            event.accepted = true;
        } else if (event.key === Qt.Key_2) {
            activeTab = 1;
            event.accepted = true;
        } else if (event.key === Qt.Key_3) {
            activeTab = 2;
            event.accepted = true;
        } else if (event.key === Qt.Key_4) {
            activeTab = 3;
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: GatewayTheme.bgMain

        // 1. HEADER BAR
        HeaderPanel {
            id: headerPanel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
        }

        // 2. TAB NAVIGATION BAR
        NavTabBar {
            id: navBar
            currentTab: rootWindow.activeTab
            anchors.top: headerPanel.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            onTabSelected: (index) => rootWindow.activeTab = index
        }

        // 3. MAIN CONTENT VIEWS DRIVEN BY STACKLAYOUT
        StackLayout {
            id: viewStack
            anchors.top: navBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: footerPanel.top
            currentIndex: rootWindow.activeTab

            SensorsOverview {}
            TrafficMonitor {}
            SystemNetwork {}
            ConfigView {
                id: configView
            }
        }

        // 4. FOOTER STATUS BAR WITH ANCHORED LEFT/RIGHT ROWS
        Rectangle {
            id: footerPanel
            height: 3
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "double"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgFooter

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: "STATUS: GATEWAY OPERATIONAL ●"
                    color: GatewayTheme.textSuccess
                    bold: true
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: "KEYS: [1-4] Tabs  [S] Save  [Q] Quit"
                    color: GatewayTheme.textAccent
                    bold: true
                }
            }
        }
    }
}
