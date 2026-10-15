pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// StackView: Push and pop pages. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Item {
        id: stackDemo
        anchors.fill: parent
        Column {
            spacing: 1
            Text {
                text: "push() adds a page on top, pop() returns to the one beneath. Depth: " + stack.depth
                color: "#8b949e"
            }
            Row {
                spacing: 2
                Button {
                    text: "Push"
                    focus: true
                    onClicked: stack.push(stackPage, {
                        level: stack.depth + 1
                    })
                }
                Button {
                    text: "Pop"
                    onClicked: stack.pop()
                }
            }
            Rectangle {
                width: 50
                height: 5
                border.style: Border.Single
                border.color: "#30363d"
                StackView {
                    id: stack
                    anchors.fill: parent
                    anchors.margins: 1
                    Component.onCompleted: push(stackPage, {
                        level: 1
                    })
                }
            }
        }
        Component {
            id: stackPage
            Column {
                property int level: 0
                Text {
                    text: "Page " + level
                    bold: true
                    color: "#e879f9"
                }
                Text {
                    text: level === 1 ? "The first page; Pop does nothing here." : "Pushed with { level: " + level + " }"
                    color: "#c9d1d9"
                }
            }
        }
    }
}
