import QtQml
import Qtui

Rectangle {
    id: control
    property string text: ""
    property bool pressed: false
    property string textColor: "white"
    property string normalColor: "transparent"
    property string focusColor: "transparent"
    property string normalBorderColor: "white"
    property string focusBorderColor: "yellow"
    property string normalBorderStyle: "single"
    property string focusBorderStyle: "double"
    property string disabledTextColor: "gray"
    property string disabledBorderColor: "gray"
    signal clicked()

    focusable: enabled
    implicitWidth: btnText.implicitWidth + 4
    implicitHeight: 3
    
    color: activeFocus ? focusColor : normalColor
    border.style: height >= 3 ? (activeFocus ? focusBorderStyle : normalBorderStyle) : "none"
    border.color: !enabled ? disabledBorderColor : (activeFocus ? focusBorderColor : normalBorderColor)
    
    Text {
        id: btnText
        anchors.centerIn: parent
        text: control.text
        color: control.enabled ? control.textColor : control.disabledTextColor
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
