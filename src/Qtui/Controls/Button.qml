import QtQml
import Qtui

// A focusable action using terminal colors. Set
// textColor, color, or border.* on an instance to override them.
Rectangle {
    id: control
    property string text: ""
    property bool pressed: false
    property rgba textColor: "default"
    signal clicked()

    focusable: enabled
    implicitWidth: btnText.implicitWidth + 4
    implicitHeight: 3

    color: "default"
    border.style: height >= 3 ? (activeFocus ? "double" : "single") : "none"
    border.color: "default"

    Text {
        id: btnText
        anchors.centerIn: parent
        text: control.text
        color: control.textColor
        bold: control.activeFocus
    }

    Keys.onPressed: (event) => {
        if (control.enabled && (event.key === Qt.Key_Return || event.key === Qt.Key_Space)) {
            control.clicked()
            event.accepted = true
        }
    }

    MouseArea {
        anchors.fill: parent

        onPressed: (event) => {
            control.focus = true
            control.pressed = true
        }
        onReleased: (event) => {
            control.pressed = false
        }
        onClicked: (event) => {
            control.clicked()
        }
    }
}
