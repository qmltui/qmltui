pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    id: root
    focus: true
    width: 50
    height: 16

    // Background content to demonstrate transparency vs occlusion
    Column {
        anchors.fill: parent
        spacing: 1

        Text { text: "=== POPUP RUNNER FIXTURE HEADER ==="; color: "cyan" }
        Text { text: "Row 1: Underneath popup background text sample"; color: "gray" }
        Text { text: "Row 2: Background text line 2 ABCDEFGHIJKLMNOPQRSTUVWXYZ"; color: "gray" }
        Text { text: "Row 3: Background text line 3 01234567890123456789"; color: "gray" }
        Button { id: bgButton; text: "Background Button"; focus: true }
        Text { text: "Row 5: Background text line 5 abcdefghijklmnopqrstuvwxyz"; color: "gray" }
        Text { text: "Row 6: Another background line for transparency check"; color: "gray" }
        Text { text: "Row 7: Bottom background content row"; color: "darkgray" }
    }

    // 1. Transparent Popup (unpainted Item, no background rectangle, just a button child)
    Popup {
        id: transPopup
        x: 6
        y: 2
        width: 38
        height: 6
        modal: true
        dimBackground: false
        closePolicy: Popup.CloseOnEscape

        Button {
            id: transBtn
            x: 4
            y: 2
            text: "Floating Button"
            focus: true
        }
    }

    // 2. Opaque Popup (composed Rectangle with double border and solid color)
    Popup {
        id: opaquePopup
        x: 6
        y: 2
        width: 38
        height: 6
        modal: true
        dimBackground: false
        closePolicy: Popup.CloseOnEscape

        Rectangle {
            id: opaqueBg
            anchors.fill: parent
            color: "#223344"
            border.style: Border.Double
            border.color: "yellow"
        }

        Text {
            x: 2
            y: 2
            text: "Opaque Composed Content"
            color: "white"
        }

        Keys.onPressed: event => {
            if (event.key === Qt.Key_R) {
                opaquePopup.width = 44
                opaquePopup.height = 8
                event.accepted = true
            }
        }
    }

    // 3. Nested Popups (parent and child popup)
    Popup {
        id: nestedParent
        x: 4
        y: 1
        width: 42
        height: 10
        modal: true
        dimBackground: false
        closePolicy: Popup.CloseOnEscape

        Rectangle {
            anchors.fill: parent
            color: "#182430"
            border.style: Border.Single
            border.color: "cyan"
        }

        Text {
            x: 2
            y: 1
            text: "Parent Popup Window"
            color: "cyan"
        }

        Keys.onPressed: event => {
            if (event.key === Qt.Key_C) {
                nestedChild.open()
                event.accepted = true
            }
        }

        Popup {
            id: nestedChild
            owner: nestedParent
            x: 8
            y: 3
            width: 26
            height: 5
            modal: true
            dimBackground: false
            closePolicy: Popup.CloseOnEscape

            Rectangle {
                anchors.fill: parent
                color: "#301824"
                border.style: Border.Double
                border.color: "magenta"
            }

            Text {
                x: 2
                y: 1
                text: "Nested Child Overlay"
                color: "magenta"
            }
        }
    }

    // 4. Default Dialog
    Dialog {
        id: defaultDialog
        title: "Standard Notice"
        message: "Default styled surface."
        showRejectButton: false
    }

    // 5. Customized Dialog
    Dialog {
        id: customDialog
        title: "Customized Dialog"
        message: "Customized background surface."
        background.color: "#1c2b3c"
        background.border.style: Border.Single
        background.border.color: "green"
        showRejectButton: false
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_1) {
            transPopup.open()
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            opaquePopup.open()
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            nestedParent.open()
            event.accepted = true
        } else if (event.key === Qt.Key_4) {
            defaultDialog.open()
            event.accepted = true
        } else if (event.key === Qt.Key_5) {
            customDialog.open()
            event.accepted = true
        } else if (event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }
}
