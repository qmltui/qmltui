pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    Rectangle {
        anchors.fill: parent
        border.style: Border.Single
        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1
            RowLayout {
                width: parent.width
                Text { text: "Layout and text"; Layout.fillWidth: true }
                Text { text: "Esc quits" }
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "A paragraph wraps at the available cell width. Resize the terminal to see these words move across lines without changing the source text."
            }
            Row {
                spacing: 2
                Text { spans: [{ text: "Ready", bold: true }, { text: " to build", color: "cyan" }] }
                Text { text: "Integer cells" }
            }
        }
    }
}
