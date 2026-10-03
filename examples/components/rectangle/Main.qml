import QtQuick
import Qtui
import Qtui.Canvas

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    color: "black"
    focus: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_M || event.key === Qt.Key_Space) {
            if (canvas.renderMode === Canvas.HalfBlock) {
                canvas.renderMode = Canvas.Braille
            } else {
                canvas.renderMode = Canvas.HalfBlock
            }
            event.accepted = true
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderMode: Canvas.HalfBlock
        color: "#ffffff"
        fillColor: "#0f172a"

        // Vector graphics shapes rendered on the canvas
        CanvasRectangle {
            x: 4; y: 4; width: 30; height: 20
            fill: false
            color: "#ef4444"
        }

        CanvasRectangle {
            x: 20; y: 12; width: 32; height: 24
            fill: true
            color: "#3b82f6"
        }

        Circle {
            cx: 60; cy: 20; radius: 14
            fill: true
            color: "#22c55e"
        }

        Circle {
            cx: 45; cy: 35; radius: 10
            fill: false
            color: "#eab308"
        }

        Line {
            x1: 4; y1: 50; x2: 90; y2: 6
            color: "#ec4899"
        }

        Polygon {
            points: [
                {x: 70, y: 38},
                {x: 90, y: 50},
                {x: 55, y: 50}
            ]
            closed: true
            color: "#a855f7"
        }
    }
}
