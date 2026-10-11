pragma ComponentBehavior: Bound
import QtQml
import Qamui

// Canvas: Animated Braille shapes. One tile of the Qamui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Item {
        id: canvasDemo
        anchors.fill: parent
        property real t: 0
        Timer {
            interval: 50
            running: true
            repeat: true
            onTriggered: canvasDemo.t += 0.08
        }
        function wave(w, h) {
            const pts = []
            for (let x = 0; x < w; ++x)
                pts.push({ x: x, y: Math.floor(h / 2 + Math.sin(x / w * Math.PI * 4 + canvasDemo.t) * h * 0.35) })
            return pts
        }
        Text { text: "Each cell holds a 2×4 Braille dot matrix; several Canvases can share the same area."; color: "#6e7b8b" }
        Rectangle {
            y: 2
            width: parent.width
            height: parent.height - 2
            color: "#020617"
            border.style: "single"
            border.color: "#1e293b"
            Canvas {
                id: waveCanvas
                anchors.fill: parent
                anchors.margins: 1
                renderMode: Canvas.Braille
                color: "#00f0ff"
                Polygon { closed: false; points: canvasDemo.wave(waveCanvas.pixelWidth, waveCanvas.pixelHeight) }
            }
            Canvas {
                id: orbitCanvas
                anchors.fill: parent
                anchors.margins: 1
                renderMode: Canvas.Braille
                color: "#ffb000"
                readonly property int cx: Math.floor(pixelWidth / 2 + Math.cos(canvasDemo.t * 0.7) * pixelWidth * 0.3)
                readonly property int cy: Math.floor(pixelHeight / 2 + Math.sin(canvasDemo.t * 0.7) * pixelHeight * 0.3)
                Circle { cx: orbitCanvas.cx; cy: orbitCanvas.cy; radius: 6; fill: false }
                Line { x1: orbitCanvas.cx - 10; y1: orbitCanvas.cy; x2: orbitCanvas.cx + 10; y2: orbitCanvas.cy }
                Line { x1: orbitCanvas.cx; y1: orbitCanvas.cy - 8; x2: orbitCanvas.cx; y2: orbitCanvas.cy + 8 }
            }
            Canvas {
                id: diagonalCanvas
                anchors.fill: parent
                anchors.margins: 1
                renderMode: Canvas.Braille
                color: "#65e6a0"
                Line { x1: 0; y1: diagonalCanvas.pixelHeight - 1; x2: diagonalCanvas.pixelWidth - 1; y2: 0 }
            }
        }
    }
}
