import QtQml 2.15
import Terminal 1.0
import Terminal.Widgets 1.0

Terminal {
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

        TreeView {
            focus: true
            width: 58
            height: 12
            borderStyle: "single"
            borderColor: "gray"

            model: {
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
                    }
                ]
            }

            delegate: Item {
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool isCurrent
                required property string display
                required property var modelData

                Row {
                    Text {
                        text: "  ".repeat(depth)
                    }

                    Text {
                        text: hasChildren ? (expanded ? "📂 " : "📁 ") : ((modelData && modelData.icon) ? modelData.icon : "📄 ")
                        color: hasChildren ? "yellow" : "cyan"
                    }

                    Text {
                        text: display
                        color: isCurrent ? "white" : "gray"
                        bold: isCurrent
                    }

                    Text {
                        text: (modelData && modelData.status) ? " [" + modelData.status + "]" : ""
                        color: (modelData && modelData.status === "Updated") ? "green" : ((modelData && modelData.status === "Modified") ? "magenta" : "gray")
                    }
                }
            }

            onNodeActivated: (path) => {
                statusText.text = "Selected Path: " + path
            }
        }

        Text {
            id: statusText
            text: "Status: Navigate tree using Qt Quick QML delegate"
            color: "cyan"
        }
    }
}
