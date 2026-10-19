pragma ComponentBehavior: Bound
import QmlTui

// GridView: Virtualized 2D grid laying out delegates in cellWidth × cellHeight cells.
Column {
    spacing: 1

    Text {
        text: "GridView arranges delegates into rows of cellWidth × cellHeight cells."
        color: "#8b949e"
    }

    Rectangle {
        width: Math.min(50, parent.width)
        height: 8
        border {
            style: Border.Single
            color: "#fbbf24"
        }

        GridView {
            anchors {
                fill: parent
                margins: 1
            }
            focus: true
            clip: true
            cellWidth: 14
            cellHeight: 2
            model: [
                {
                    glyph: "🍎",
                    name: "apple"
                },
                {
                    glyph: "🍋",
                    name: "lemon"
                },
                {
                    glyph: "🍇",
                    name: "grape"
                },
                {
                    glyph: "🍓",
                    name: "berry"
                },
                {
                    glyph: "🥝",
                    name: "kiwi"
                },
                {
                    glyph: "🍑",
                    name: "peach"
                },
                {
                    glyph: "🍒",
                    name: "cherry"
                },
                {
                    glyph: "🥭",
                    name: "mango"
                },
                {
                    glyph: "🍍",
                    name: "pineapple"
                },
                {
                    glyph: "🫐",
                    name: "blueberry"
                },
                {
                    glyph: "🍈",
                    name: "melon"
                },
                {
                    glyph: "🍐",
                    name: "pear"
                }
            ]

            delegate: Row {
                id: cell
                required property int index
                required property var modelData

                spacing: 1

                Text {
                    text: cell.modelData.glyph
                }

                Text {
                    text: cell.modelData.name
                    color: cell.index % 2 === 0 ? "#c9d1d9" : "#fbbf24"
                }
            }
        }
    }
}
