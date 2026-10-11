import QtQml
import Qamui
import Qamui.Controls

Terminal {
    id: root

    title: "Boot to Qamui — Device Setup"
    color: Theme.bgMain
    focus: true

    property int activePage: 0
    property string clock: Qt.formatTime(new Date(), "hh:mm")

    function focusActivePage() {
        if (root.activePage === 0) {
            networkPage.focus = true;
        } else {
            root.focus = true;
        }
    }

    // Deferred so the StackLayout has shown the page before it takes focus.
    onActivePageChanged: Qt.callLater(root.focusActivePage)
    Component.onCompleted: {
        Theme.simpleGlyphs = WifiManager.asciiConsole;
        root.focusActivePage();
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.clock = Qt.formatTime(new Date(), "hh:mm")
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
            return;
        }
        const pageKeys = [
            [Qt.Key_F1, Qt.Key_1],
            [Qt.Key_F2, Qt.Key_2],
        ];
        for (let index = 0; index < pageKeys.length; ++index) {
            if (pageKeys[index].includes(event.key)) {
                root.activePage = index;
                event.accepted = true;
                return;
            }
        }
    }

    // Top brand bar: Qt logo block, title, system status, and clock
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: Theme.bgHeader
        border.color: Theme.borderNormal

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 2
            spacing: 1

            Rectangle {
                width: 4
                height: 1
                color: Theme.qtGreen

                Text {
                    anchors.centerIn: parent
                    text: "Qt"
                    color: Theme.textOnGreen
                    bold: true
                }
            }

            Text {
                text: "Boot to Qamui · Device Setup"
                color: Theme.textPrimary
                bold: true
            }

            Text {
                text: "· " + WifiManager.hostname
                color: Theme.textSecondary
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 2
            spacing: 2

            StatusBadge {
                text: WifiManager.connected ? Theme.iconWifi + WifiManager.currentSsid : Theme.iconWifi + "Offline"
                badgeColor: WifiManager.connected ? Theme.qtGreen : Theme.statusWarn
                showIcon: false
            }

            Text {
                text: root.clock
                color: Theme.textPrimary
                bold: true
            }
        }
    }

    Rectangle {
        id: navFrame
        height: 3
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        border.style: "single"
        border.color: Theme.borderNormal

        TabBar {
            id: navBar
            anchors.fill: parent
            anchors.margins: 1
            tabs: ["[F1] Network", "[F2] About"]
            currentIndex: root.activePage
            onCurrentIndexChanged: root.activePage = currentIndex
        }
    }

    StackLayout {
        anchors.top: navFrame.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        currentIndex: root.activePage

        NetworkPage {
            id: networkPage
            terminal: root
        }

        AboutPage {}
    }

    Rectangle {
        id: footer
        height: 2
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.bgHeader
        border.color: Theme.borderNormal

        Text {
            anchors.centerIn: parent
            text: {
                if (root.activePage === 0) {
                    if (WifiManager.connected)
                        return "[F1/F2] Pages  [Q/Esc] Quit";
                    return Theme.keyUpDown + " Select AP  [Enter] Connect  [S/R] Scan  [F1/F2] Pages  [Q/Esc] Quit";
                }
                return "[F1/F2] Pages  [Q/Esc] Quit";
            }
            color: Theme.textSecondary
        }
    }
}
