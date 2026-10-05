pragma ComponentBehavior: Bound

import QtQml
import Qtui


Terminal {
    id: root

    property string selectedPath

    width: 60
    height: 18

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "🌲 QT QUICK NATIVE TREEVIEW DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Use UP/DOWN to navigate, RIGHT/LEFT to toggle, Q/ESC to quit"
            color: "gray"
        }
 
        Rectangle {
            width: parent.width
            height: parent.height - 6
            border.style: "single"
            border.color: "gray"

            TreeView {
                focus: true
                anchors.fill: parent
                anchors.margins: 1

                rootNode: {
                    "text": "qtui (Workspace Root)",
                    "expanded": true,
                    "children": [
                        {
                            "text": "Terminal (Core Library)",
                            "expanded": true,
                            "children": [
                                {
                                    "text": "src (Source Files)",
                                    "expanded": true,
                                    "children": [
                                        { "text": "TextGridBuffer.cpp", "icon": "📄", "status": "Clean", "expanded": false },
                                        { "text": "TerminalEngine.cpp", "icon": "📄", "status": "Active", "expanded": false },
                                        { "text": "TerminalDevice.cpp", "icon": "📄", "status": "Modified", "expanded": false }
                                    ]
                                }
                            ]
                        },
                        {
                            "text": "Widgets (Controls & Views)",
                            "expanded": true,
                            "children": [
                                { "text": "TableView.cpp", "icon": "📊", "status": "Clean", "expanded": false },
                                { "text": "TreeView.cpp", "icon": "🌲", "status": "Updated", "expanded": false },
                                { "text": "ToastManager.cpp", "icon": "🔔", "status": "Clean", "expanded": false }
                            ]
                        },
                    ]
                }

                onNodeActivated: (path) => {
                    root.selectedPath = path
                }

                delegate: Item {
                    id: treeDelegate

                    required property TreeView treeView
                    required property bool isTreeNode
                    required property int index
                    required property int row
                    required property int column
                    required property bool current
                    required property bool selected
                    required property int depth
                    required property bool hasChildren
                    required property bool expanded
                    required property string display
                    required property string icon
                    required property string status

                    Row {
                        Text {
                            text: "  ".repeat(treeDelegate.depth)
                        }

                        Text {
                            text: treeDelegate.hasChildren
                                ? (treeDelegate.expanded ? "📂 " : "📁 ")
                                : (treeDelegate.icon !== "" ? treeDelegate.icon + " " : "📄 ")
                            color: treeDelegate.hasChildren ? "yellow" : "cyan"
                        }

                        Text {
                            text: treeDelegate.display
                            color: treeDelegate.current ? "white" : "gray"
                            bold: treeDelegate.current
                        }

                        Text {
                            text: treeDelegate.status !== "" ? " [" + treeDelegate.status + "]" : ""
                            color: treeDelegate.status === "Updated"
                                ? "green" : (treeDelegate.status === "Modified" ? "magenta" : "gray")
                        }
                    }
                }
            }
        }

        Text {
            id: statusText
            text: root.selectedPath === ""
                ? "Status: Navigate tree using Qt Quick QML delegate"
                : "Selected Path: " + root.selectedPath
            color: "cyan"
        }
    }
}
