pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// A tree whose branches load on first expansion. The status line counts the
// fetchMore() calls the TreeView issued; nothing is fetched until you expand.
Terminal {
    id: root

    LazyDirectoryModel { id: files }

    Column {
        anchors.fill: parent

        Text {
            width: parent.width
            text: "Lazy model: Enter/Right expands a branch and fetches its rows. q quits."
            color: "cyan"
        }

        TreeView {
            id: tree
            width: parent.width
            height: parent.height - 2
            model: files
            focus: true
            delegate: Item {
                id: node
                required property int depth
                required property bool expanded
                required property bool hasChildren
                required property bool current
                required property string display
                required property string size
                width: tree.width
                height: 1
                Text {
                    width: parent.width
                    color: node.hasChildren ? "yellow" : "default"
                    reversed: node.current
                    text: " ".repeat(node.depth * 2) + (node.hasChildren ? (node.expanded ? "v " : "> ") : "  ")
                          + node.display + "  " + node.size
                }
            }
        }

        Text {
            width: parent.width
            text: "fetchMore calls: " + files.fetchCount + "   rows loaded: " + files.loadedCount
            color: "gray"
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
}
