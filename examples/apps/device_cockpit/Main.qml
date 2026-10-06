import QtQml
import Qtui
import Qtui.Controls
import Qtui.Extras
import Qtui.Examples.DeviceCockpit

Terminal {
    id: root
    color: Theme.bgMain

    property int activePage: 0

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }
        const pageKeys = [
            [Qt.Key_F1, Qt.Key_1],
            [Qt.Key_F2, Qt.Key_2],
            [Qt.Key_F3, Qt.Key_3],
            [Qt.Key_F4, Qt.Key_4],
        ]
        for (let index = 0; index < pageKeys.length; ++index) {
            if (pageKeys[index].includes(event.key)) {
                root.activePage = index
                event.accepted = true
                return
            }
        }
    }

    HeaderPanel {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Rectangle {
        id: navFrame
        height: 3
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        border.style: "single"
        border.color: Theme.borderNormal

        TabHeader {
            id: navBar
            anchors.fill: parent
            anchors.margins: 1
            tabs: ["[F1] Overview", "[F2] Network", "[F3] Services", "[F4] Logs"]
            currentIndex: root.activePage
            activeTabColor: Theme.textPrimary
            activeTabBgColor: Theme.bgActiveTab
            inactiveTabColor: Theme.textSecondary
            onCurrentIndexChanged: root.activePage = currentIndex
        }
    }

    StackLayout {
        anchors.top: navFrame.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        currentIndex: root.activePage

        OverviewPage {}
        NetworkPage { toasts: toastManager }
        ServicesPage { toasts: toastManager }
        LogsPage {}
    }

    Rectangle {
        id: footer
        height: 3
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.bgHeader
        border.style: "double"
        border.color: Theme.borderNormal

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            text: "MODE: " + (CockpitBackend.mockMode ? "SIMULATED DEVICE" : "LIVE SYSTEM")
            color: CockpitBackend.mockMode ? Theme.textWarning : Theme.textAccent
            bold: true
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            text: "[F1-F4] Pages  [Q] Quit"
            color: Theme.textSecondary
        }
    }

    ToastManager {
        id: toastManager
        anchors.fill: parent
        position: "bottom-right"
        toastWidth: 44
    }
}
