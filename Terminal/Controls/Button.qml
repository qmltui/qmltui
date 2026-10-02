import QtQml 2.15
import Terminal 1.0

Rectangle {
    id: control
    property string label: ""
    property bool pressed: false
    signal clicked()

    // Style configuration
    property string textColor: "white"
    property string normalColor: "transparent"
    property string focusColor: "transparent"
    property string normalBorderColor: "white"
    property string focusBorderColor: "yellow"
    property string normalBorderStyle: "single"
    property string focusBorderStyle: "double"

    focusable: true
    width: label.length + 4
    height: 3
    
    color: activeFocus ? focusColor : normalColor
    border.style: activeFocus ? focusBorderStyle : normalBorderStyle
    border.color: activeFocus ? focusBorderColor : normalBorderColor
    
    Text {
        anchors.centerIn: parent
        text: control.label
        color: control.textColor
    }

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
