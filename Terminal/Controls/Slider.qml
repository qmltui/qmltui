import QtQml 2.15
import Terminal 1.0

Rectangle {
    id: control
    property real value: 0.0
    property real from: 0.0
    property real to: 100.0
    property real stepSize: 1.0
    property string trackColor: "gray"
    property string handleColor: activeFocus ? "yellow" : "cyan"
    property string textColor: "white"
    property bool showValue: true

    focusable: true
    width: 25
    height: 1
    color: "transparent"

    property int trackWidth: width - (showValue ? 7 : 0)

    Text {
        anchors.verticalCenter: parent.verticalCenter
        x: 0
        text: {
            var tw = Math.max(1, control.trackWidth)
            var range = Math.max(0.001, control.to - control.from)
            var pct = Math.min(1.0, Math.max(0.0, (control.value - control.from) / range))
            var handlePos = Math.round(pct * (tw - 1))
            
            var str = ""
            for (var i = 0; i < tw; ++i) {
                if (i === handlePos) {
                    str += "●"
                } else {
                    str += "─"
                }
            }
            return str
        }
        color: control.activeFocus ? control.handleColor : control.trackColor
        bold: control.activeFocus
    }

    Text {
        visible: control.showValue
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        text: "[" + Math.round(control.value) + "]"
        color: control.textColor
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Left) {
            control.value = Math.max(control.from, control.value - control.stepSize)
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            control.value = Math.min(control.to, control.value + control.stepSize)
            event.accepted = true
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (event) => {
            control.focus = true
            var tw = Math.max(1, control.trackWidth)
            var pct = Math.min(1.0, Math.max(0.0, event.x / tw))
            control.value = Math.round(control.from + pct * (control.to - control.from))
        }
    }
}
