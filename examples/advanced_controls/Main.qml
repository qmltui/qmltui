import QtQml 2.15
import Tui 1.0
import Tui.Controls 1.0

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    width: 80
    height: 24
    backgroundColor: "black"
    useAlternateScreen: true

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title Header
        Text {
            width: parent.width
            height: 3
            text: "Concept Demo: Advanced Controls, ScrollView & Borders"
            color: "yellow"
            backgroundColor: "#222222"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }

        // Horizontal Row splits form vs scroll list
        Row {
            width: parent.width
            height: 15
            spacing: 2

            // Form container on the left with Single Border
            Column {
                width: 36
                height: parent.height
                border.style: "single"
                border.color: "cyan"
                spacing: 1
                padding: 1

                Text {
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    text: "INPUT FORM"
                    bold: true
                    color: "cyan"
                }

                Text {
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    text: "Enter text below:"
                    color: "gray"
                }

                // TextInput component
                TextInput {
                    id: inputField
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    height: 3
                    border.style: activeFocus ? "double" : "single"
                    border.color: activeFocus ? "yellow" : "white"
                    text: "Type here"
                    color: "white"

                    onAccepted: {
                        statusText.text = "Submitted via Enter: " + text
                    }
                }

                // Button control
                Button {
                    label: "Submit Form"
                    onClicked: {
                        statusText.text = "Submitted via Click: " + inputField.text
                    }
                }
            }

            // Scroll list container on the right with Double Border
            Column {
                width: 40
                height: parent.height
                border.style: "double"
                border.color: "green"
                spacing: 1
                padding: 1

                Text {
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    text: "SCROLLABLE LIST (Mouse / Up-Down)"
                    bold: true
                    color: "green"
                }

                ScrollView {
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    height: parent.height - parent.topPadding - parent.bottomPadding - 2
                    contentHeight: 20
                    border.style: "ascii"
                    border.color: "gray"

                    Column {
                        width: parent.width - parent.leftPadding - parent.rightPadding
                        spacing: 0

                        Text { width: parent.width; text: "[01] Scroll line one"; color: "white" }
                        Text { width: parent.width; text: "[02] Scroll line two"; color: "silver" }
                        Text { width: parent.width; text: "[03] Scroll line three"; color: "gray" }
                        Text { width: parent.width; text: "[04] Scroll line four"; color: "white" }
                        Text { width: parent.width; text: "[05] Scroll line five"; color: "silver" }
                        Text { width: parent.width; text: "[06] Scroll line six"; color: "gray" }
                        Text { width: parent.width; text: "[07] Scroll line seven"; color: "white" }
                        Text { width: parent.width; text: "[08] Scroll line eight"; color: "silver" }
                        Text { width: parent.width; text: "[09] Scroll line nine"; color: "gray" }
                        Text { width: parent.width; text: "[10] Scroll line ten"; color: "white" }
                        Text { width: parent.width; text: "[11] Scroll line eleven"; color: "silver" }
                        Text { width: parent.width; text: "[12] Scroll line twelve"; color: "gray" }
                        Text { width: parent.width; text: "[13] Scroll line thirteen"; color: "white" }
                        Text { width: parent.width; text: "[14] Scroll line fourteen"; color: "silver" }
                        Text { width: parent.width; text: "[15] Scroll line fifteen"; color: "gray" }
                    }
                }
            }
        }

        // Bottom status area
        Text {
            id: statusText
            width: parent.width
            text: "Status: Ready. Use Tab to move between TextInput, Button, and ScrollView."
            color: "magenta"
        }

        Text {
            width: parent.width
            text: "Press 'q' to exit this example."
            color: "silver"
        }
    }
}
