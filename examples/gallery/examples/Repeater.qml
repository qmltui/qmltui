pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int accentIndex: 0
    property var nodes: [
        {
            name: "node-01",
            role: "ingress"
        },
        {
            name: "node-02",
            role: "worker"
        },
        {
            name: "node-03",
            role: "worker"
        },
        {
            name: "node-04",
            role: "storage"
        }
    ]

    readonly property var accentColors: ["#2cde85", "#38bdf8", "#c4b5fd"]
    readonly property var accentNames: ["Emerald", "Cyan", "Violet"]
    readonly property string accentColor: accentColors[accentIndex]
    readonly property string accentName: accentNames[accentIndex]

    function addNode() {
        if (nodes.length >= 6) {
            return;
        }
        const nextNum = String(nodes.length + 1).padStart(2, "0");
        const roles = ["ingress", "worker", "storage", "cache", "search", "metrics"];
        const copy = nodes.slice();
        copy.push({
            name: "node-" + nextNum,
            role: roles[(nodes.length) % roles.length]
        });
        nodes = copy;
    }

    function removeNode() {
        if (nodes.length <= 1) {
            return;
        }
        nodes = nodes.slice(0, nodes.length - 1);
    }

    function cycleAccent() {
        accentIndex = (accentIndex + 1) % accentColors.length;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal || event.key === Qt.Key_A) {
                root.addNode();
                event.accepted = true;
            } else if (event.key === Qt.Key_Minus || event.key === Qt.Key_D) {
                root.removeNode();
                event.accepted = true;
            } else if (event.key === Qt.Key_C) {
                root.cycleAccent();
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Repeater Node Matrix"
                    color: "#c4b5fd"
                }
                Text {
                    text: "[Core]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.nodes.length + " nodes"
                color: root.accentColor
            }
            footer: Text {
                text: "+/A add · -/D remove · C accent"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Repeater"
                color: "#8b949e"
            }
            GridLayout {
                x: 2
                y: 2
                width: Math.max(24, parent.width - 4)
                height: 13
                columns: 2
                columnSpacing: 1
                rowSpacing: 1

                Repeater {
                    model: root.nodes
                    delegate: Rectangle {
                        id: card

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 3
                        color: "#161b22"
                        border {
                            style: Border.Single
                            color: root.accentColor
                        }

                        Text {
                            x: 2
                            y: 1
                            text: "#" + (card.index + 1) + " " + card.modelData.name + " · " + card.modelData.role
                            color: "#f0f6fc"
                            bold: true
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Inspector"
                color: "#58a6ff"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Total Nodes: " + root.nodes.length
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "Accent: " + root.accentName
                    color: root.accentColor
                    bold: true
                }

                Button {
                    id: addBtn

                    focus: true
                    text: "+ Add Node"
                    textColor: "#2cde85"
                    onClicked: root.addNode()
                }

                Button {
                    id: removeBtn

                    text: "- Remove Node"
                    textColor: "#f87171"
                    onClicked: root.removeNode()
                }

                Button {
                    id: accentBtn

                    text: "Cycle Accent"
                    textColor: "#c4b5fd"
                    onClicked: root.cycleAccent()
                }
            }
        }
    }
}
