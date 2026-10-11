pragma ComponentBehavior: Bound
import Qamui

// GridView: Cells over a model. One tile of the Qamui gallery (examples/apps/gallery)
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
        x: 2
        y: 1
        spacing: 1
        Text { text: "GridView lays delegates out in cellWidth × cellHeight cells and scrolls like a ScrollView."; color: "#6e7b8b" }
        Rectangle {
            width: 66
            height: 8
            border.style: "single"
            border.color: "#fbbf24"
            GridView {
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                cellWidth: 16
                cellHeight: 2
                model: [
                    { glyph: "🍎", name: "apple" }, { glyph: "🍋", name: "lemon" }, { glyph: "🍇", name: "grape" }, { glyph: "🍓", name: "berry" },
                    { glyph: "🥝", name: "kiwi" }, { glyph: "🍑", name: "peach" }, { glyph: "🍒", name: "cherry" }, { glyph: "🥭", name: "mango" },
                    { glyph: "🍍", name: "pineapple" }, { glyph: "🫐", name: "blueberry" }, { glyph: "🍈", name: "melon" }, { glyph: "🍐", name: "pear" }
                ]
                delegate: Row {
                    id: cell
                    required property int index
                    required property var modelData
                    spacing: 1
                    Text { text: cell.modelData.glyph }
                    Text { text: cell.modelData.name; color: cell.index % 2 === 0 ? "#c9d1d9" : "#fbbf24" }
                }
            }
        }
    }
}
