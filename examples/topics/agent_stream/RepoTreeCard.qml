import QtQml
import Qamui

Column {
    id: treeCard
    required property string title
    width: parent ? parent.width : 76
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "🌲 Workspace Hierarchy:"
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " " + (treeCard.title || "~/Qamui")
            color: "white"
            bold: true
        }
    }

    TreeView {
        id: repoTreeView
        width: parent.width > 0 ? Math.min(parent.width, 76) : 76
        height: 7
        textColor: "white"
        branchColor: "#38bdf8"

        Component.onCompleted: {
            repoTreeView.rootNode = {
                "text": "Qamui (Repository Root)",
                "expanded": true,
                "children": [
                    {
                        "text": "src/Qamui/engine",
                        "expanded": true,
                        "children": [
                            { "text": "StreamRenderer.cpp [Optimized]", "expanded": false },
                            { "text": "TextGridBuffer.cpp [Verified]", "expanded": false }
                        ]
                    },
                    {
                        "text": "examples/topics/agent_stream",
                        "expanded": true,
                        "children": [
                            { "text": "Main.qml [Active]", "expanded": false },
                            { "text": "PinnedPromptBar.qml [Clean]", "expanded": false }
                        ]
                    }
                ]
            }
        }
    }
}
