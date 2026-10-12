import QtQml
import QmlTui
import QmlTui.Controls

// Session banner: a Canvas mark beside the product and model details.
Row {
    id: banner
    required property string activeModel
    property int logoRenderMode: Canvas.HalfBlock
    width: parent ? parent.width : 80
    spacing: 2

    Canvas {
        visible: banner.logoRenderMode === Canvas.HalfBlock
        width: 11
        height: 5
        renderMode: Canvas.HalfBlock
        color: "#f8fafc"

        // Center the nine-pixel-wide mark with one pixel of padding on each side.
        Circle { cx: 5; cy: 4; radius: 3; fill: false }
        Circle { cx: 5; cy: 4; radius: 4; fill: false }
        // One surface preserves independent colors in the two halves of a cell.
        Polygon {
            closed: false
            color: "#00ff66"
            points: [{ x: 6, y: 5 }, { x: 8, y: 7 }, { x: 6, y: 9 }]
        }
        Polygon {
            closed: false
            color: "#00ff66"
            points: [{ x: 7, y: 5 }, { x: 9, y: 7 }, { x: 7, y: 9 }]
        }
    }

    Item {
        visible: banner.logoRenderMode === Canvas.Braille
        width: 11
        height: 5

        Canvas {
            anchors.fill: parent
            renderMode: Canvas.Braille
            color: "#f8fafc"

            // Adjacent outlines form a thick ring without painting its hollow center.
            Circle { cx: 11; cy: 8; radius: 5; fill: false }
            Circle { cx: 11; cy: 8; radius: 6; fill: false }
            Circle { cx: 11; cy: 8; radius: 7; fill: false }
            Circle { cx: 11; cy: 8; radius: 8; fill: false }
        }

        // Separate glyph layers keep green from recoloring white circle dots.
        // The chevron occludes the circle in terminal cells shared by both.
        Canvas {
            anchors.fill: parent
            renderMode: Canvas.Braille
            color: "#00ff66"

            // Polygon strokes are one dot wide; layer parallel chevrons for density.
            Polygon {
                closed: false
                points: [{ x: 12, y: 10 }, { x: 16, y: 14 }, { x: 12, y: 18 }]
            }
            Polygon {
                closed: false
                points: [{ x: 13, y: 10 }, { x: 17, y: 14 }, { x: 13, y: 18 }]
            }
            Polygon {
                closed: false
                points: [{ x: 14, y: 10 }, { x: 18, y: 14 }, { x: 14, y: 18 }]
            }
            Polygon {
                closed: false
                points: [{ x: 15, y: 10 }, { x: 19, y: 14 }, { x: 15, y: 18 }]
            }
        }
    }

    Column {
        spacing: 0
        Row {
            spacing: 1
            Text {
                text: "QmlTui"
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
            text: "✦ Qt AI studio · " + banner.activeModel + " · ~/QmlTui"
            color: "#94a3b8"
        }
        Text {
            text: "Type a prompt or /demo [1-4] to simulate agent workflows"
            color: "#64748b"
        }
        Text {
            text: "/help for commands · [F1] persistent · [F2] model"
            color: "#64748b"
        }
        Text {
            text: "Ctrl+Q quits · Esc quits when the prompt is empty"
            color: "#64748b"
        }
    }
}
