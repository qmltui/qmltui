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
        Rectangle {
            width: parent.width
            height: 3
            color: "#222222"

            Text {
                anchors.fill: parent
                text: "Concept Demo: Advanced Controls, ScrollView & Borders"
                color: "yellow"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Horizontal Row splits form vs scroll list
        Row {
            width: parent.width
            height: 15
            spacing: 2

            // Form container on the left with Single Border
            Rectangle {
                width: 36
                height: parent.height
                border.style: "single"
                border.color: "cyan"
                padding: 1

                Column {
                    anchors.fill: parent
                    spacing: 1

                    Text {
                        width: parent.width
                        text: "INPUT FORM"
                        bold: true
                        color: "cyan"
                    }

                    Text {
                        width: parent.width
                        text: "Enter text below:"
                        color: "gray"
                    }

                    // TextInput component wrapped in styled Rectangle
                    Rectangle {
                        width: parent.width
                        height: 3
                        border.style: inputField.activeFocus ? "double" : "single"
                        border.color: inputField.activeFocus ? "yellow" : "white"

                        TextInput {
                            id: inputField
                            anchors.fill: parent
                            anchors.margins: 1
                            text: "Type here"
                            color: "white"

                            onAccepted: {
                                statusText.text = "Submitted via Enter: " + text
                            }
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
            }

            // Scroll list container on the right with Double Border
            Rectangle {
                width: 40
                height: parent.height
                border.style: "double"
                border.color: "green"
                padding: 1

                Column {
                    anchors.fill: parent
                    spacing: 1

                    Text {
                        width: parent.width
                        text: "SCROLLABLE LIST (Mouse / Up-Down)"
                        bold: true
                        color: "green"
                    }

                    Rectangle {
                        width: parent.width
                        height: parent.height - 2
                        border.style: "ascii"
                        border.color: "gray"
                        padding: 1

                        ScrollView {
                            anchors.fill: parent
                            contentHeight: 20

                            Column {
                                width: parent.width
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
