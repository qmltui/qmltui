pragma ComponentBehavior: Bound
import QmlTui

// MouseArea: Clicks, presses, hover. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        id: mouseDemo
        x: 2
        y: 1
        property string last: "nothing yet"
        spacing: 1
        Text { text: "Click inside the box; hover to see the pointer position (needs a mouse-capable terminal)."; color: "#6e7b8b" }
        Rectangle {
            id: pad
            width: 50
            height: 7
            color: area.containsPress ? "#7c2d12" : "#1c1917"
            border.style: Border.Single
            border.color: "#f97316"
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onMouseXChanged: x => { mouseDemo.last = "x at " + x; }
                onMouseYChanged: y=> { mouseDemo.last = "y at " + y;  }
                onClicked: event => {
                    const name = event.button === Qt.RightButton ? "right" : "left";
                    mouseDemo.last = name + " click at (" + event.x + ", " + event.y + ")";
                    event.accepted = true;
                }
            }
            Text { anchors.centerIn: parent; text: area.pressed ? "pressed" : "hover " + area.mouseX + ", " + area.mouseY; color: "#f97316" }
        }
        Text { text: "Last: " + mouseDemo.last; color: "#f97316" }
    }
}
