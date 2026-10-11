import QtQml
import Qamui
import Qamui.Controls

// Session banner: the Qamui mark drawn by the library itself (a braille
// Canvas: an O and a > prompt whose upper arm crosses the ring to make the Q)
// next to the product line and the model in use.
Row {
    id: banner
    required property string activeModel
    width: parent ? parent.width : 80
    spacing: 2

    Canvas {
        id: mark
        width: 10
        height: 5
        renderMode: Canvas.Braille
        cellColorMode: Canvas.LastDot
        color: "#f8fafc"

        // 20 x 20 braille dots: a two-dot ring and a two-dot chevron.
        Circle { cx: 8; cy: 9; radius: 7; fill: false }
        Circle { cx: 8; cy: 9; radius: 6; fill: false }
        Polygon {
            closed: false
            color: "#00ff66"
            points: [{ x: 11, y: 11 }, { x: 15, y: 15 }, { x: 11, y: 19 }]
        }
        Polygon {
            closed: false
            color: "#00ff66"
            points: [{ x: 12, y: 11 }, { x: 16, y: 15 }, { x: 12, y: 19 }]
        }
    }

    Column {
        spacing: 0
        Row {
            spacing: 1
            Text {
                text: "Qamui"
                color: "#f8fafc"
                bold: true
            }
            StatusBadge {
                text: "v0.5"
                status: "info"
                variant: "subtle"
            }
            Text {
                text: "· agent stream demo"
                color: "#38bdf8"
            }
        }
        Text {
            text: "✦ Qt AI studio · " + banner.activeModel + " · ~/Qamui"
            color: "#94a3b8"
        }
        Text {
            text: "Type a prompt or /demo [1-4] to simulate agent workflows · /help for commands"
            color: "#64748b"
        }
        Text {
            text: "Ctrl+Q quits · Esc quits when the prompt is empty"
            color: "#64748b"
        }
    }
}
