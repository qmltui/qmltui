pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    readonly property var modules: [
        {
            code: "TERM",
            icon: "▣",
            accent: "#2cde85",
            desc: "AltScreen & Inline root"
        },
        {
            code: "TEXT",
            icon: "¶",
            accent: "#38bdf8",
            desc: "Spans, Rich & ANSI text"
        },
        {
            code: "RECT",
            icon: "╭",
            accent: "#60a5fa",
            desc: "Box borders & gradients"
        },
        {
            code: "TABL",
            icon: "▤",
            accent: "#fb923c",
            desc: "Connected grid TableView"
        },
        {
            code: "TREE",
            icon: "▾",
            accent: "#86efac",
            desc: "Hierarchical TreeView"
        },
        {
            code: "LIST",
            icon: "≡",
            accent: "#f472b6",
            desc: "Virtualized ListView"
        },
        {
            code: "GRID",
            icon: "▦",
            accent: "#fbbf24",
            desc: "2D cell GridView"
        },
        {
            code: "LOGS",
            icon: "▼",
            accent: "#a3e635",
            desc: "Streaming tail LogView"
        },
        {
            code: "INPT",
            icon: "█",
            accent: "#4ade80",
            desc: "Single-line TextInput"
        },
        {
            code: "AREA",
            icon: "✎",
            accent: "#a78bfa",
            desc: "Highlighted TextArea"
        },
        {
            code: "CANV",
            icon: "⣿",
            accent: "#22d3ee",
            desc: "Braille vector Canvas"
        },
        {
            code: "CTRL",
            icon: "◈",
            accent: "#e879f9",
            desc: "Buttons, Dialogs & Tabs"
        }
    ]
    readonly property var activeModule: gridView.currentIndex >= 0 && gridView.currentIndex < modules.length ? modules[gridView.currentIndex] : modules[0]
    readonly property int tileCol: gridView.currentIndex % gridView.cols
    readonly property int tileRow: Math.floor(gridView.currentIndex / gridView.cols)

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "GridView · Module Matrix"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#fbbf24"
            titleTrailing: (gridView.currentIndex + 1) + "/" + root.modules.length
            titleTrailingColor: "#8b949e"
            footer: "↑↓←→ navigate tiles · click to select"
            footerColor: "#8b949e"
            footerTrailing: gridView.cellWidth + "x" + gridView.cellHeight
            footerTrailingColor: "#58a6ff"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            GridView {
                id: gridView
                property int currentIndex: 0
                readonly property int cols: Math.max(1, Math.floor(width / cellWidth))
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                cellWidth: 12
                cellHeight: 3
                reuseItems: true
                model: root.modules

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
                        gridView.currentIndex = Math.min(root.modules.length - 1, gridView.currentIndex + 1);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
                        gridView.currentIndex = Math.max(0, gridView.currentIndex - 1);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
                        gridView.currentIndex = Math.min(root.modules.length - 1, gridView.currentIndex + gridView.cols);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
                        gridView.currentIndex = Math.max(0, gridView.currentIndex - gridView.cols);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Home) {
                        gridView.currentIndex = 0;
                        event.accepted = true;
                    } else if (event.key === Qt.Key_End) {
                        gridView.currentIndex = root.modules.length - 1;
                        event.accepted = true;
                    }
                }

                delegate: Rectangle {
                    id: tile
                    required property int index
                    required property string code
                    required property string icon
                    required property string accent
                    readonly property bool active: index === gridView.currentIndex
                    width: 11
                    height: 3
                    color: active ? "#12261e" : "#161b22"
                    border {
                        style: active ? Border.Double : Border.Rounded
                        color: active ? tile.accent : "#30363d"
                    }

                    Text {
                        anchors {
                            centerIn: parent
                        }
                        text: tile.icon + " " + tile.code
                        bold: tile.active
                        color: tile.active ? tile.accent : "#c9d1d9"
                    }

                    MouseArea {
                        anchors {
                            fill: parent
                        }
                        onClicked: {
                            gridView.currentIndex = tile.index;
                            gridView.focus = true;
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "GridView API"
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
                    text: "SELECTED TILE"
                    bold: true
                    color: "#fbbf24"
                }

                Text {
                    text: "currentIndex: " + gridView.currentIndex
                    color: "#c9d1d9"
                }

                Text {
                    text: "Module: " + root.activeModule.icon + " " + root.activeModule.code
                    bold: true
                    color: root.activeModule.accent
                }

                Text {
                    text: "Coord:  (" + root.tileCol + ", " + root.tileRow + ")"
                    color: "#38bdf8"
                }

                Text {
                    text: "CellXY: (" + (root.tileCol * gridView.cellWidth) + ", " + (root.tileRow * gridView.cellHeight) + ")"
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "GRID GEOMETRY"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "cellSize: " + gridView.cellWidth + "x" + gridView.cellHeight
                    color: "#c9d1d9"
                }

                Text {
                    text: "columns:  " + gridView.cols
                    color: "#c9d1d9"
                }

                Text {
                    text: "count:    " + root.modules.length
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "DESCRIPTION"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    width: parent.width
                    text: root.activeModule.desc
                    wrapMode: Text.WordWrap
                    color: "#f8fafc"
                }
            }
        }
    }
}
