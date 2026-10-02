import QtQml
import Tui

Terminal {
    id: rootWindow

    function handleItemKey(itemName, event) {
        const navigationKey = event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab
        const quitKey = event.key === Qt.Key_Escape || event.key === Qt.Key_Q
        if (navigationKey || quitKey)
            return

        logText.text = qsTr("%1 received key: %2 (code: %3)")
                       .arg(itemName)
                       .arg(event.text)
                       .arg(event.key)
        event.accepted = true
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    width: 80
    height: 24
    backgroundColor: "black"
    useAlternateScreen: true

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Header
        Rectangle {
            width: parent.width
            height: 3
            color: "#222222"

            Text {
                anchors.fill: parent
                text: qsTr("Concept Demo: Focus Management & Keyboard Input")
                color: "yellow"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        Text {
            width: parent.width
            text: qsTr("Press Tab or Shift-Tab to cycle focus between the three items.\nFocused items will highlight. Press any key while focused.")
            color: "gray"
        }

        // Focusable items
        Row {
            spacing: 3
            width: parent.width

            // Item 1
            Rectangle {
                id: item1
                width: 20
                height: 3
                focusable: true
                color: activeFocus ? "blue" : "#111111"

                Text {
                    anchors.fill: parent
                    text: item1.activeFocus ? qsTr("[*] Focus Item A") : qsTr("    Focus Item A")
                    color: item1.activeFocus ? "white" : "silver"
                    bold: item1.activeFocus
                    verticalAlignment: Qt.AlignVCenter
                }

                Keys.onPressed: (event) => {
                    rootWindow.handleItemKey(qsTr("Item A"), event)
                }
            }

            // Item 2
            Rectangle {
                id: item2
                width: 20
                height: 3
                focusable: true
                color: activeFocus ? "green" : "#111111"

                Text {
                    anchors.fill: parent
                    text: item2.activeFocus ? qsTr("[*] Focus Item B") : qsTr("    Focus Item B")
                    color: item2.activeFocus ? "white" : "silver"
                    bold: item2.activeFocus
                    verticalAlignment: Qt.AlignVCenter
                }

                Keys.onPressed: (event) => {
                    rootWindow.handleItemKey(qsTr("Item B"), event)
                }
            }

            // Item 3
            Rectangle {
                id: item3
                width: 20
                height: 3
                focusable: true
                color: activeFocus ? "magenta" : "#111111"

                Text {
                    anchors.fill: parent
                    text: item3.activeFocus ? qsTr("[*] Focus Item C") : qsTr("    Focus Item C")
                    color: item3.activeFocus ? "white" : "silver"
                    bold: item3.activeFocus
                    verticalAlignment: Qt.AlignVCenter
                }

                Keys.onPressed: (event) => {
                    rootWindow.handleItemKey(qsTr("Item C"), event)
                }
            }
        }

        // Logger label
        Text {
            id: logText
            width: parent.width
            text: qsTr("No key pressed yet.")
            color: "cyan"
        }

        Text {
            width: parent.width
            text: qsTr("Press 'q' to exit this example.")
            color: "silver"
        }
    }
}
