pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// TreeView: Expandable hierarchical tree navigated with Up/Down and Left/Right.
Column {
    id: treeDemo

    property string selected: ""

    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Up/Down move, Right/Left expand and collapse, Enter activates a node."
        color: "#8b949e"
    }

    Rectangle {
        width: 48
        height: 10
        color: "#161b22"

        TreeView {
            anchors {
                fill: parent
                margins: 1
            }
            focus: true
            rootNode: {
                "text": "src",
                "icon": "",
                "expanded": true,
                "children": [
                    {
                        "text": "Core",
                        "icon": "",
                        "expanded": true,
                        "children": [
                            {
                                "text": "Item.cpp",
                                "icon": "📄"
                            },
                            {
                                "text": "Text.cpp",
                                "icon": "📄"
                            },
                            {
                                "text": "TableView.cpp",
                                "icon": "📄"
                            }
                        ]
                    },
                    {
                        "text": "Controls",
                        "icon": "",
                        "expanded": false,
                        "children": [
                            {
                                "text": "Button.qml",
                                "icon": "📄"
                            },
                            {
                                "text": "Dialog.qml",
                                "icon": "📄"
                            }
                        ]
                    },
                    {
                        "text": "CMakeLists.txt",
                        "icon": "⚙"
                    }
                ]
            }
            onNodeActivated: path => {
                treeDemo.selected = path;
            }

            // Custom row delegate receiving depth, expanded, and current state
            delegate: Row {
                id: node
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool current
                required property string display
                required property string icon

                Text {
                    text: "  ".repeat(node.depth)
                }

                Text {
                    text: node.hasChildren ? (node.expanded ? "▾ " : "▸ ") : (node.icon !== "" ? node.icon + " " : "  ")
                    color: node.hasChildren ? "#f2cc60" : "#2cde85"
                }

                Text {
                    text: node.display
                    color: node.current ? "#00ff66" : "#c9d1d9"
                    bold: node.current
                }
            }
        }
    }

    Text {
        text: treeDemo.selected === "" ? "Nothing activated yet" : "Activated: " + treeDemo.selected
        color: "#2cde85"
    }
}
