pragma ComponentBehavior: Bound
import QmlTui

// Animation: Number and Color, easing. One tile of the QmlTui gallery (examples/gallery)
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
        x: 2
        y: 1
        spacing: 1
        Text { text: "Two NumberAnimations and a ColorAnimation loop forever with different easing curves."; color: "#6e7b8b" }
        Text { text: "Easing.OutBounce"; color: "#c9d1d9" }
        Item {
            width: 60; height: 1
            Text { text: "─".repeat(60); color: "#1e293b" }
            Text {
                id: bouncer
                text: "████"
                color: "#f9a8d4"
                NumberAnimation { target: bouncer; property: "x"; from: 0; to: 56; duration: 1600; loops: -1; running: true; easing: Easing.OutBounce }
            }
        }
        Text { text: "Easing.InOutQuad"; color: "#c9d1d9" }
        Item {
            width: 60; height: 1
            Text { text: "─".repeat(60); color: "#1e293b" }
            Text {
                id: glider
                text: "◆◆◆◆"
                color: "#c084fc"
                NumberAnimation { target: glider; property: "x"; from: 56; to: 0; duration: 1600; loops: -1; running: true; easing: Easing.InOutQuad }
            }
        }
        Text {
            id: pulse
            text: "ColorAnimation: this text fades between two colours"
            bold: true
            color: "#f9a8d4"
            ColorAnimation { target: pulse; property: "color"; from: "#f9a8d4"; to: "#312e81"; duration: 1600; loops: -1; running: true; easing: Easing.InOutQuad }
        }
        Text { text: "x = " + bouncer.x + " / " + glider.x; color: "#f9a8d4" }
    }
}
