pragma ComponentBehavior: Bound
import QmlTui

// ScrollView: Scroll any tall content. One tile of the QmlTui gallery (examples/gallery)
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
        Text { text: "Up/Down, PageUp/PageDown or the wheel scroll; the bar on the right shows the position."; color: "#6e7b8b" }
        Rectangle {
            width: 50
            height: 9
            border.style: Border.Single
            border.color: "#67e8f9"
            ScrollView {
                id: scroller
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                Column {
                    Repeater {
                        model: 40
                        delegate: Text {
                            required property int index
                            text: "line " + (index + 1) + (index % 10 === 9 ? "  ── section " + Math.floor(index / 10 + 1) : "")
                            color: index % 10 === 9 ? "#67e8f9" : "#c9d1d9"
                        }
                    }
                }
            }
        }
        Text { text: "contentY " + scroller.contentY + " of " + scroller.contentHeight; color: "#67e8f9" }
    }
}
