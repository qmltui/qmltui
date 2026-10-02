import QtQml 2.15
import Terminal 1.0
import Terminal.Controls 1.0

Terminal {
    id: rootWindow

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

 

    // Main Content Area
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        spacing: 2

        // Left Panel - Form Controls
        Panel {
            width: 37
            height: parent.height
            title: "Settings & Toggles"
            titleAlignment: Qt.AlignLeft
            titleColor: "cyan"
            border.color: "blue"
            border.style: "double"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                CheckBox {
                    label: "Enable Auto-Save"
                    checked: true
                    onToggled: (val) => statusText.text = "Auto-Save: " + val
                }

                CheckBox {
                    label: "Show Line Numbers"
                    checked: false
                    onToggled: (val) => statusText.text = "Line Numbers: " + val
                }

                Text { text: "─────────────────────────────────"; color: "gray" }
   
                ButtonGroup {
                    buttons: [radio1, radio2]
                }

                RadioButton {
                    id: radio1
                    label: "Mode: Normal"
                    checked: true
                    onToggled: (val) => statusText.text = "Selected Normal Mode"
                }

                RadioButton {
                    id: radio2
                    label: "Mode: Developer"
                    checked: false
                    onToggled: (val) => statusText.text = "Selected Developer Mode"
                }

                Text { text: "─────────────────────────────────"; color: "gray" }

                Switch {
                    label: "Dark Theme"
                    checked: true
                    onToggled: (val) => statusText.text = "Dark Theme: " + val
                }
            }
        }

        // Right Panel - Sliders and Buttons
        Panel {
            width: 37
            height: parent.height
            title: "Adjustments"
            titleAlignment: Qt.AlignHCenter
            titleColor: "yellow"
            border.color: "green"
            border.style: "single"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text { text: "Volume Level:"; color: "white" }

                Slider {
                    width: 33
                    value: 65
                    from: 0
                    to: 100
                }

                Text { text: "Brightness:"; color: "white" }

                Slider {
                    width: 33
                    value: 40
                    from: 0
                    to: 100
                }

                Text { text: "─────────────────────────────────"; color: "gray" }

                Button {
                    label: "Save Configuration"
                    onClicked: statusText.text = "Configuration saved!"
                }

                Text {
                    id: statusText
                    width: parent.width
                    text: "Status: Ready."
                    color: "magenta"
                }
            }
        }
    }

}
