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
        Text {
            width: parent.width
            height: 3
            text: qsTr("Concept Demo: Focus Management & Keyboard Input")
            color: "yellow"
            backgroundColor: "#222222"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
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
            Text {
                id: item1
                width: 20
                height: 3
                focusable: true
                text: activeFocus ? qsTr("[*] Focus Item A") : qsTr("    Focus Item A")
                backgroundColor: activeFocus ? "blue" : "#111111"
                color: activeFocus ? "white" : "silver"
                bold: activeFocus

                Keys.onPressed: (event) => {
                    rootWindow.handleItemKey(qsTr("Item A"), event)
                }
            }

            // Item 2
            Text {
                id: item2
                width: 20
                height: 3
                focusable: true
                text: activeFocus ? qsTr("[*] Focus Item B") : qsTr("    Focus Item B")
                backgroundColor: activeFocus ? "green" : "#111111"
                color: activeFocus ? "white" : "silver"
                bold: activeFocus

                Keys.onPressed: (event) => {
                    rootWindow.handleItemKey(qsTr("Item B"), event)
                }
            }

            // Item 3
            Text {
                id: item3
                width: 20
                height: 3
                focusable: true
                text: activeFocus ? qsTr("[*] Focus Item C") : qsTr("    Focus Item C")
                backgroundColor: activeFocus ? "magenta" : "#111111"
                color: activeFocus ? "white" : "silver"
                bold: activeFocus

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
