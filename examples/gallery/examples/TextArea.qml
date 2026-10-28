pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// TextArea: Multi-line text editor with 2D scrolling, cursor navigation, and selection.
ColumnLayout {
    anchors.fill: parent
    spacing: 1

    Text {
        text: "Enter inserts a newline; arrow keys move the cursor."
        color: "#8b949e"
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#161b22"

        TextArea {
            id: notes
            anchors {
                fill: parent
                margins: 1
            }
            focus: true
            color: "#c9d1d9"
            placeholderText: "Write a few lines…"
            text: "# QmlTui Multi-Line Editor\n\nQmlTui renders declarative QML scenes directly into terminal cells.\nThis TextArea maintains a scrollable multi-line document buffer.\n\nKey Features:\n  - 2D cursor navigation (Arrows, Home/End, PageUp/PageDown)\n  - Text selection with Shift+Arrow or mouse drag\n  - Clipboard copy, cut, and bracketed paste support\n  - Undo and redo history stack\n  - Custom syntax highlight layers (used by the Gallery code editor)\n\nTry editing these lines, inserting new paragraphs with Enter,\nor selecting a block of text to see the live character and line counts update below."

            Component.onCompleted: {
                notes.setHighlightLayer("heading", [
                    {
                        "start": 0,
                        "length": 26,
                        "color": "#00ff66",
                        "bold": true
                    }
                ], notes.textRevision);
            }
        }
    }

    Text {
        text: notes.text.length + " characters, " + notes.text.split("\n").length + " lines"
        color: "#2cde85"
    }
}
