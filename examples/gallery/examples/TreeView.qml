pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int treeRevision: 0
    property bool asciiIcons: false
    property string lastActivated: "none"
    readonly property int activeDepth: treeRevision >= 0 && treeView.currentIndex >= 0 ? treeView.depth(treeView.currentIndex) : 0
    readonly property bool activeExpanded: treeRevision >= 0 && treeView.currentIndex >= 0 ? treeView.isExpanded(treeView.currentIndex) : false
    readonly property string activeName: treeView.currentItemData && treeView.currentItemData.text ? String(treeView.currentItemData.text) : "qml-tui"

    function expandAllNodes() {
        treeView.expandAll();
        root.treeRevision += 1;
    }

    function collapseAllNodes() {
        treeView.collapseAll();
        root.treeRevision += 1;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_E) {
                root.expandAllNodes();
                event.accepted = true;
            } else if (event.key === Qt.Key_C) {
                root.collapseAllNodes();
                event.accepted = true;
            } else if (event.key === Qt.Key_I) {
                root.asciiIcons = !root.asciiIcons;
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "TreeView · Project Explorer"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#86efac"
            titleTrailing: "row " + treeView.currentIndex
            titleTrailingColor: "#8b949e"
            footer: "↑↓←→ nav · Space toggle · E expand · C collapse"
            footerColor: "#8b949e"
            footerTrailing: "d:" + root.activeDepth
            footerTrailingColor: "#58a6ff"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            TreeView {
                id: treeView
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                currentIndex: 0
                textColor: "#c9d1d9"
                branchColor: "#58a6ff"
                selectionColor: "#2cde85"
                selectionBackgroundColor: "#12261e"
                expandedIcon: root.asciiIcons ? "[-] " : "▾ "
                collapsedIcon: root.asciiIcons ? "[+] " : "▸ "
                rootNode: ({
                        text: "qml-tui",
                        expanded: true,
                        children: [
                            {
                                text: "src/QmlTui/Core",
                                expanded: true,
                                children: [
                                    {
                                        text: "Terminal.cpp"
                                    },
                                    {
                                        text: "TableView.cpp"
                                    },
                                    {
                                        text: "TreeView.cpp"
                                    }
                                ]
                            },
                            {
                                text: "src/QmlTui/Controls",
                                expanded: false,
                                children: [
                                    {
                                        text: "Panel.qml"
                                    },
                                    {
                                        text: "Button.qml"
                                    }
                                ]
                            },
                            {
                                text: "examples/gallery",
                                expanded: true,
                                children: [
                                    {
                                        text: "Main.qml"
                                    },
                                    {
                                        text: "TreeView.qml"
                                    }
                                ]
                            },
                            {
                                text: "docs/qml-types",
                                expanded: false,
                                children: [
                                    {
                                        text: "treeview.md"
                                    },
                                    {
                                        text: "tableview.md"
                                    }
                                ]
                            }
                        ]
                    })

                onCurrentIndexChanged: root.treeRevision += 1
                onExpanded: (row, depth) => {
                    root.treeRevision += 1;
                }
                onCollapsed: (row, recursively) => {
                    root.treeRevision += 1;
                }
                onNodeActivated: path => {
                    const parts = path.split("/");
                    root.lastActivated = parts[parts.length - 1];
                    root.treeRevision += 1;
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "TreeView API"
            footerColor: "#8b949e"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "ACTIVE NODE"
                    bold: true
                    color: "#86efac"
                }

                Text {
                    text: "currentIndex: " + treeView.currentIndex
                    color: "#c9d1d9"
                }

                Text {
                    text: "depth: " + root.activeDepth
                    color: "#38bdf8"
                }

                Text {
                    text: "expanded: " + (root.activeExpanded ? "true" : "false")
                    color: root.activeExpanded ? "#2cde85" : "#fbbf24"
                }

                Text {
                    width: parent.width
                    text: "Node: " + root.activeName
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    width: parent.width
                    text: "Last: " + root.lastActivated
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "BRANCH ICONS"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "Preset: " + (root.asciiIcons ? "[+] / [-]" : "▸ / ▾")
                    color: "#c9d1d9"
                }

                CheckBox {
                    text: "ASCII icons (I)"
                    checked: root.asciiIcons
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.asciiIcons = checked;
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "TREE ACTIONS"
                    bold: true
                    color: "#58a6ff"
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ Expand All (E) ]"
                    textColor: "#2cde85"
                    onClicked: root.expandAllNodes()
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ Collapse All (C) ]"
                    textColor: "#fbbf24"
                    onClicked: root.collapseAllNodes()
                }
            }
        }
    }
}
