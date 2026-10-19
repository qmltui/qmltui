pragma ComponentBehavior: Bound
import QmlTui

// TextArea: Multi-line text editor with 2D scrolling, cursor navigation, and selection.
Column {
    spacing: 1

    Text {
        text: "Enter inserts a newline; arrow keys move the cursor."
        color: "#8b949e"
    }

    Rectangle {
        width: Math.min(54, parent.width)
        height: 7
        border {
            style: Border.Single
            color: "#a78bfa"
        }

        TextArea {
            id: notes
            anchors {
                fill: parent
                margins: 1
            }
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
