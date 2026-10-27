pragma ComponentBehavior: Bound
import QmlTui

// MouseArea: Captures mouse clicks, right-clicks, press states, and pointer hover.
Column {
    id: mouseDemo

    property string last: "nothing yet"
    property int pointerX: 24
    property int pointerY: 4
    property int pinX: -1
    property int pinY: -1

    function syncPointer() {
        pointerX = Math.max(1, Math.min(pad.width - 2, area.mouseX));
        pointerY = Math.max(1, Math.min(pad.height - 2, area.mouseY));
        last = (area.pressed ? "drag (" : "hover (") + pointerX + ", " + pointerY + ")";
    }

    anchors.centerIn: parent
    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Text {
        text: "Move or click inside the box to track the cursor:"
        color: "#8b949e"
    }

    Rectangle {
        id: pad
        objectName: "mousePad"
        width: 50
        height: 9
        color: area.containsPress ? "#122d24" : "#161b22"

        // Horizontal crosshair guide line
        Rectangle {
            x: 1
            y: mouseDemo.pointerY
            width: pad.width - 2
            height: 1
            color: "#21262d"
        }

        // Vertical crosshair guide line
        Rectangle {
            x: mouseDemo.pointerX
            y: 1
            width: 1
            height: pad.height - 2
            color: "#21262d"
        }

        // Last clicked pin marker
        Text {
            visible: mouseDemo.pinX >= 1 && mouseDemo.pinY >= 1
            x: Math.max(1, Math.min(pad.width - 2, mouseDemo.pinX))
            y: Math.max(1, Math.min(pad.height - 2, mouseDemo.pinY))
            text: "×"
            color: "#00ff66"
            bold: true
        }

        // Live cursor marker that follows mouse movements
        Text {
            id: cursorMarker
            x: mouseDemo.pointerX
            y: mouseDemo.pointerY
            text: area.pressed ? "◉" : "┼"
            color: area.pressed ? "#00ff66" : "#2cde85"
            bold: true
        }

        // Coordinate badge next to the cursor marker
        Text {
            readonly property string label: "(" + mouseDemo.pointerX + "," + mouseDemo.pointerY + ")"
            x: mouseDemo.pointerX + 2 + label.length < pad.width - 1 ? mouseDemo.pointerX + 2 : Math.max(1, mouseDemo.pointerX - label.length - 1)
            y: mouseDemo.pointerY
            text: label
            color: "#c9d1d9"
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onPressed: mouse => {
                mouseDemo.focus = true;
                mouseDemo.pointerX = Math.max(1, Math.min(pad.width - 2, mouse.x));
                mouseDemo.pointerY = Math.max(1, Math.min(pad.height - 2, mouse.y));
            }
            onMouseXChanged: mouseDemo.syncPointer()
            onMouseYChanged: mouseDemo.syncPointer()
            onClicked: event => {
                const name = event.button === Qt.RightButton ? "right" : "left";
                mouseDemo.pinX = Math.max(1, Math.min(pad.width - 2, event.x));
                mouseDemo.pinY = Math.max(1, Math.min(pad.height - 2, event.y));
                mouseDemo.last = name + " click at (" + mouseDemo.pinX + ", " + mouseDemo.pinY + ")";
                event.accepted = true;
            }
        }
    }

    Text {
        text: "Last: " + mouseDemo.last
        color: "#2cde85"
    }
}
