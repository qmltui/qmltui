pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property bool brailleMode: true
    property int phase: 0
    readonly property int stepX: brailleMode ? 2 : 1
    readonly property int stepY: brailleMode ? 4 : 2
    readonly property int cellW: Math.max(12, vectorCanvas.width)
    readonly property int cellH: Math.max(6, vectorCanvas.height)
    readonly property int dividerCellX: Math.max(6, Math.floor(cellW * 0.44))
    readonly property int diamondStartCellX: Math.max(dividerCellX + 6, Math.floor(cellW * 0.72))
    readonly property int circleMinX: (dividerCellX + 2) * stepX
    readonly property int circleMaxX: Math.max(circleMinX + 4, (diamondStartCellX - 1) * stepX - 1)
    readonly property int maxShapeRy: Math.max(2, Math.floor(((cellH - 2) * stepY - 2) / 2))

    function toggleMode() {
        root.brailleMode = !root.brailleMode;
    }

    function stepPhase() {
        root.phase = (root.phase + 1) % 12;
    }

    function sineWavePoints(pw, ph, p) {
        const pts = [];
        const x0 = 2 * root.stepX;
        const x1 = Math.max(x0 + 2, (root.dividerCellX - 1) * root.stepX - 1);
        const yMin = root.stepY + 1;
        const yMax = Math.max(yMin + 2, (root.cellH - 1) * root.stepY - 2);
        const midY = Math.floor((yMin + yMax - 1) / 2);
        const amp = Math.max(2, Math.min(root.maxShapeRy, Math.floor(ph * 0.34)));
        const steps = 20;
        for (let i = 0; i <= steps; ++i) {
            const px = Math.min(x1, Math.max(x0, Math.round(x0 + (i / steps) * (x1 - x0))));
            const angle = (i / steps) * Math.PI * 2.0 + p * 0.52;
            const py = Math.min(yMax, Math.max(yMin, Math.round(midY + Math.sin(angle) * amp)));
            pts.push(Qt.point(px, py));
        }
        return pts;
    }

    function diamondPoints(pw, ph) {
        const dMinX = root.diamondStartCellX * root.stepX;
        const dMaxX = Math.max(dMinX + 4, (root.cellW - 2) * root.stepX - 1);
        const cx = Math.floor((dMinX + dMaxX) / 2);
        const cy = Math.floor(ph * 0.5);
        const rx = Math.max(2, Math.min(Math.floor((dMaxX - dMinX) / 2), Math.floor(ph * 0.26)));
        const ry = Math.max(2, Math.min(root.maxShapeRy, Math.floor(ph * 0.28)));
        return [Qt.point(cx, cy - ry), Qt.point(cx + rx, cy), Qt.point(cx, cy + ry), Qt.point(cx - rx, cy)];
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_M) {
                root.toggleMode();
                event.accepted = true;
            } else if (event.key === Qt.Key_Space) {
                root.stepPhase();
                event.accepted = true;
            }
        }

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Canvas"
                    color: "#00f0ff"
                }
                Text {
                    text: root.brailleMode ? "[Braille 2x4]" : "[HalfBlock 1x2]"
                    color: "#2cde85"
                }
            }
            titleTrailing: Text {
                text: vectorCanvas.pixelWidth + "x" + vectorCanvas.pixelHeight + " px"
                color: "#8b949e"
            }
            footer: Text {
                text: "[M] RenderMode  [Space] Advance Phase"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "phase #" + root.phase
                color: "#58a6ff"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    width: parent.width
                    clip: true
                    text: "Shapes: Sine Wave · Circle · Line · CanvasRectangle · Polygon"
                    color: "#8b949e"
                }

                Canvas {
                    id: vectorCanvas
                    width: parent.width
                    height: 14
                    renderMode: root.brailleMode ? Canvas.Braille : Canvas.HalfBlock
                    color: "#58a6ff"

                    CanvasRectangle {
                        x: 0
                        y: 0
                        width: Math.max(4, vectorCanvas.pixelWidth)
                        height: Math.max(4, vectorCanvas.pixelHeight)
                        color: "#30363d"
                        fill: false
                    }

                    Line {
                        x1: root.dividerCellX * root.stepX
                        y1: root.stepY + 1
                        x2: root.dividerCellX * root.stepX
                        y2: Math.max(root.stepY + 2, (root.cellH - 1) * root.stepY - 2)
                        color: "#484f58"
                    }

                    Circle {
                        cx: Math.floor((root.circleMinX + root.circleMaxX) / 2)
                        cy: Math.floor(vectorCanvas.pixelHeight * 0.5)
                        radius: Math.max(2, Math.min(Math.floor((root.circleMaxX - root.circleMinX) / 2), Math.min(root.maxShapeRy, Math.floor(vectorCanvas.pixelHeight * 0.25))))
                        color: "#f9c74f"
                        fill: false
                    }

                    Polygon {
                        color: "#f472b6"
                        closed: true
                        points: root.diamondPoints(vectorCanvas.pixelWidth, vectorCanvas.pixelHeight)
                    }

                    Polygon {
                        color: "#00f0ff"
                        closed: false
                        points: root.sineWavePoints(vectorCanvas.pixelWidth, vectorCanvas.pixelHeight, root.phase)
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
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "renderMode:"
                    color: "#8b949e"
                }

                Text {
                    text: root.brailleMode ? "  Canvas.Braille" : "  Canvas.HalfBlock"
                    bold: true
                    color: "#00f0ff"
                }

                Text {
                    text: "cells: " + vectorCanvas.width + "x" + vectorCanvas.height
                    color: "#c9d1d9"
                }

                Text {
                    text: "pixels: " + vectorCanvas.pixelWidth + "x" + vectorCanvas.pixelHeight
                    bold: true
                    color: "#2cde85"
                }

                Text {
                    text: "wave phase: " + root.phase + " rad/6"
                    color: "#f9c74f"
                }

                Text {
                    text: "primitives: 5 active"
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Toggle Mode [M]"
                    onClicked: root.toggleMode()
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Step Wave [Space]"
                    onClicked: root.stepPhase()
                }
            }
        }
    }
}
