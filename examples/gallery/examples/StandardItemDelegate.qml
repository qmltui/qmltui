pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// StandardItemDelegate: Default row for any view. One tile of the QmlTui gallery (examples/gallery)
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
        Text { text: "The default row: decoration glyph, check box when the model allows it, bold reverse video when current."; color: "#6e7b8b" }
        Rectangle {
            width: 40
            height: 8
            border.style: Border.Single
            border.color: "#93c5fd"
            ListView {
                id: tasks
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                currentIndex: 0
                model: ["Build", "Run tests", "Tag release", "Publish notes", "Announce", "Celebrate"]
                delegate: StandardItemDelegate { textColor: "#93c5fd" }
            }
        }
        Text { text: tasks.currentIndex >= 0 ? "Current: " + tasks.model[tasks.currentIndex] : "No current item"; color: "#93c5fd" }
    }
}
