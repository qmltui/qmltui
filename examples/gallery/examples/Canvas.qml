pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Canvas: Sub-cell Braille (2×4 dots per cell) vector drawing surface.
ColumnLayout {
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

    anchors.fill: parent
    spacing: 1

    Timer {
        interval: 50
        running: canvasDemo.visible
        repeat: true
        onTriggered: canvasDemo.t += 0.08
    }

    Text {
        text: "Each cell holds a 2×4 Braille dot matrix."
        color: "#8b949e"
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#161b22"

        // Sine wave polyline
        Canvas {
            id: waveCanvas
            anchors {
                fill: parent
                margins: 1
            }
            renderMode: Canvas.Braille
            color: "#2cde85"
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "#00ff66"
                }
                GradientStop {
                    position: 0.5
                    color: "#56d4dd"
                }
                GradientStop {
                    position: 1.0
                    color: "#d2a8ff"
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
            color: "#f2cc60"

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
