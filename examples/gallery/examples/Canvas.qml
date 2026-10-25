pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Canvas: Sub-cell Braille (2×4 dots per cell) vector drawing surface.
Item {
    id: canvasDemo

    property real t: 0

    function wave(w, h) {
        const pts = [];
        for (let x = 0; x < w; ++x) {
            pts.push({
                x: x,
                y: Math.floor(h / 2 + Math.sin(x / w * Math.PI * 4 + canvasDemo.t) * h * 0.35)
            });
        }
        return pts;
    }

    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: canvasDemo.t += 0.08
    }

    Text {
        text: "Each cell holds a 2×4 Braille dot matrix."
        color: "#8b949e"
    }

    Rectangle {
        y: 2
        width: parent.width
        height: Math.max(6, parent.height - 2)
        color: "#020617"
        border {
            style: Border.Single
            color: "#1e293b"
        }

        // Sine wave polyline
        Canvas {
            id: waveCanvas
            anchors {
                fill: parent
                margins: 1
            }
            renderMode: Canvas.Braille
            color: "#00f0ff"
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "#00f0ff"
                }
                GradientStop {
                    position: 0.5
                    color: "#a855f7"
                }
                GradientStop {
                    position: 1.0
                    color: "#ec4899"
                }
            }

            Polygon {
                closed: false
                points: canvasDemo.wave(waveCanvas.pixelWidth, waveCanvas.pixelHeight)
            }
        }

        // Orbiting crosshair and circle
        Canvas {
            id: orbitCanvas
            readonly property int cx: Math.floor(pixelWidth / 2 + Math.cos(canvasDemo.t * 0.7) * pixelWidth * 0.3)
            readonly property int cy: Math.floor(pixelHeight / 2 + Math.sin(canvasDemo.t * 0.7) * pixelHeight * 0.3)

            anchors {
                fill: parent
                margins: 1
            }
            renderMode: Canvas.Braille
            color: "#ffb000"

            Circle {
                cx: orbitCanvas.cx
                cy: orbitCanvas.cy
                radius: 6
                fill: false
            }

            Line {
                x1: orbitCanvas.cx - 10
                y1: orbitCanvas.cy
                x2: orbitCanvas.cx + 10
                y2: orbitCanvas.cy
            }

            Line {
                x1: orbitCanvas.cx
                y1: orbitCanvas.cy - 8
                x2: orbitCanvas.cx
                y2: orbitCanvas.cy + 8
            }
        }
    }
}
