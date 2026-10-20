pragma ComponentBehavior: Bound
import QmlTui

// MouseArea: Captures mouse clicks, right-clicks, press states, and pointer hover.
Column {
    id: mouseDemo

    property string last: "nothing yet"

    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Text {
        text: "Click or hover inside the box to inspect pointer coordinates."
        color: "#8b949e"
    }

    Rectangle {
        id: pad
        width: Math.min(44, parent.width)
        height: 7
        color: area.containsPress ? "#7c2d12" : "#1c1917"
        border {
            style: Border.Single
            color: "#f97316"
        }

        Text {
            anchors.centerIn: parent
            text: area.pressed ? "pressed" : "hover " + area.mouseX + ", " + area.mouseY
            color: "#f97316"
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onPressed: {
                mouseDemo.focus = true;
            }
            onMouseXChanged: x => {
                mouseDemo.last = "x at " + x;
            }
            onMouseYChanged: y => {
                mouseDemo.last = "y at " + y;
            }
            onClicked: event => {
                const name = event.button === Qt.RightButton ? "right" : "left";
                mouseDemo.last = name + " click at (" + event.x + ", " + event.y + ")";
                event.accepted = true;
            }
        }
    }

    Text {
        text: "Last: " + mouseDemo.last
        color: "#f97316"
    }
}
