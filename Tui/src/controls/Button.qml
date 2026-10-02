import QtQml 2.15
import Tui 1.0

Text {
    id: control
    property string label: ""
    property bool pressed: false
    signal clicked()

    focusable: true
    width: label.length + 4
    height: 3
    
    border.style: activeFocus ? "double" : "single"
    border.color: activeFocus ? "yellow" : "white"
    
    text: label
    horizontalAlignment: Qt.AlignHCenter
    verticalAlignment: Qt.AlignVCenter

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
            control.clicked()
            event.accepted = true
        }
    }

    MouseArea {
        width: control.width
        height: control.height
        
        onPressed: (event) => {
            control.focus = true
            control.pressed = true
        }
        onReleased: (event) => {
            if (control.pressed) {
                control.pressed = false
                control.clicked()
            }
        }
    }
}
