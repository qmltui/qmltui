pragma ComponentBehavior: Bound
import QmlTui

// TextArea: Multi-line editing. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text {
            text: "Enter inserts a line break; arrows move the cursor; Ctrl+C copies the selection."
            color: "#8b949e"
        }
        Rectangle {
            width: 66
            height: 7
            border.style: Border.Single
            border.color: "#a78bfa"
            TextArea {
                id: notes
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                color: "#c9d1d9"
                placeholderText: "Write a few lines…"
                text: "QmlTui renders QML into terminal cells.\nThis TextArea keeps a multi-line buffer."
            }
        }
        Text {
            text: notes.text.length + " characters, " + notes.text.split("\n").length + " lines"
            color: "#a78bfa"
        }
    }
}
