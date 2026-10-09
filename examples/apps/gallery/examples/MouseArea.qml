pragma ComponentBehavior: Bound
import Qtui

// MouseArea: Clicks, presses, hover. One tile of the Qtui gallery (examples/apps/gallery)
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
            color: area.pressedNow ? "#7c2d12" : "#1c1917"
            border.style: "single"
            border.color: "#f97316"
            MouseArea {
                id: area
                property bool pressedNow: false
                anchors.fill: parent
                hoverEnabled: true
                onMouseXChanged: x => { mouseDemo.last = "x at " + x; }
                onMouseYChanged: y=> { mouseDemo.last = "y at " + y;  }
                onPressed: event => { pressedNow = true; event.accepted = true }
                onReleased: event => { pressedNow = false; event.accepted = true }
                onClicked: event => { mouseDemo.last = "click at (" + event.x + ", " + event.y + ") button " + event.button; event.accepted = true }
            }
            Text { anchors.centerIn: parent; text: area.pressedNow ? "pressed" : "hover " + area.mouseX + ", " + area.mouseY; color: "#f97316" }
        }
        Text { text: "Last: " + mouseDemo.last; color: "#f97316" }
    }
}
