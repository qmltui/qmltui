import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Qtui Button & ButtonGroup Component Example"
            color: "cyan"
        }

        Text {
            text: "Tab moves focus, Enter or Space activates, the mouse clicks."
            color: "gray"
        }

        Text {
            text: "──────────────────────────────────────────"
            color: "gray"
        }

        // hoverEnabled is opt-in: it turns on the terminal's any-motion mouse
        // reporting, so only enable it on buttons that benefit from it. The
        // label is drawn bold while the pointer is over the button; the
        // double border still marks keyboard focus.
        Button {
            id: primary
            text: "[ Primary Action ]"
            hoverEnabled: true
            onClicked: statusText.text = "Primary button clicked!"
        }

        Button {
            id: secondary
            text: "[ Secondary Action ]"
            hoverEnabled: true
            onClicked: statusText.text = "Secondary button clicked!"
        }

        Text {
            text: primary.hovered ? "Hover: Primary Action"
                 : secondary.hovered ? "Hover: Secondary Action"
                 : "Hover: none (move the mouse over a button)"
            color: "gray"
        }

        Text {
            text: "Select Option (ButtonGroup):"
            color: "yellow"
        }

        ButtonGroup {
            buttons: [btn1, btn2, btn3]
        }

        Row {
            spacing: 2
            RadioButton {
                id: btn1
                text: "Option A"
                checked: true
                onToggled: val => statusText.text = "Option A selected"
            }
            RadioButton {
                id: btn2
                text: "Option B"
                onToggled: val => statusText.text = "Option B selected"
            }
            RadioButton {
                id: btn3
                text: "Option C"
                onToggled: val => statusText.text = "Option C selected"
            }
        }

        Text {
            text: "──────────────────────────────────────────"
            color: "gray"
        }

        Text {
            id: statusText
            text: "Status: Ready."
            color: "red"
        }
    }
}
