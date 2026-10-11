pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// TreeView: Expandable node tree. One tile of the Qamui gallery (examples/apps/gallery)
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
        id: treeDemo
        x: 2
        y: 1
        property string selected: ""
        spacing: 1
        Text { text: "Up/Down move, Right/Left expand and collapse, Enter activates a node."; color: "#6e7b8b" }
        Rectangle {
            width: 50
            height: 8
            border.style: "single"
            border.color: "#86efac"
            TreeView {
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                rootNode: {
                    "text": "src", "icon": "", "expanded": true,
                    "children": [
                        { "text": "Core", "icon": "", "expanded": true, "children": [
                            { "text": "Item.cpp", "icon": "📄" },
                            { "text": "Text.cpp", "icon": "📄" },
                            { "text": "TableView.cpp", "icon": "📄" }
                        ] },
                        { "text": "Controls", "icon": "", "expanded": false, "children": [
                            { "text": "Button.qml", "icon": "📄" },
                            { "text": "Dialog.qml", "icon": "📄" }
                        ] },
                        { "text": "CMakeLists.txt", "icon": "⚙" }
                    ]
                }
                onNodeActivated: path => treeDemo.selected = path
                delegate: Row {
                    id: node
                    required property int depth
                    required property bool hasChildren
                    required property bool expanded
                    required property bool current
                    required property string display
                    required property string icon
                    Text { text: "  ".repeat(node.depth) }
                    Text {
                        text: node.hasChildren ? (node.expanded ? "▾ " : "▸ ") : (node.icon !== "" ? node.icon + " " : "  ")
                        color: node.hasChildren ? "#f9c74f" : "#86efac"
                    }
                    Text { text: node.display; color: node.current ? "#f8fafc" : "#c9d1d9"; bold: node.current }
                }
            }
        }
        Text { text: treeDemo.selected === "" ? "Nothing activated yet" : "Activated: " + treeDemo.selected; color: "#86efac" }
    }
}
