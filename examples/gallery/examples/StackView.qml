pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// StackView: Page navigation stack supporting push(), pop(), and depth tracking.
Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "push() adds a page on top, pop() returns beneath. Depth: " + stack.depth
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
            width: 44
            height: 5
            color: "#161b22"

            StackView {
                id: stack
                anchors {
                    fill: parent
                    margins: 1
                }
                Component.onCompleted: push(stackPage, {
                    level: 1
                })
            }
        }
    }

    // Reusable page Component instantiated with properties via stack.push()
    Component {
        id: stackPage

        Column {
            property int level: 0

            Text {
                text: "Page " + level
                bold: true
                color: "#00ff66"
            }

            Text {
                text: level === 1 ? "Root page; Pop does nothing here." : "Pushed with { level: " + level + " }"
                color: "#c9d1d9"
            }
        }
    }
}
