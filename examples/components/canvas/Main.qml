import QtQml 2.15
import QtQml.Models 2.15
import Qtui
import Qtui.Canvas

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "#0f172a"
    useAlternateScreen: true

    property bool paused: false
    property double animTime: 0.0
    property bool showCube: true
    property bool showSine: true
    property bool showRadar: true

    property double angleX: 0.0
    property double angleY: 0.0

    // Explicit key handling to quit, as per project rules (.agents/AGENTS.md)
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Space) {
            rootWindow.paused = !rootWindow.paused
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_1) {
            rootWindow.showCube = !rootWindow.showCube
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_2) {
            rootWindow.showSine = !rootWindow.showSine
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_3) {
            rootWindow.showRadar = !rootWindow.showRadar
            event.accepted = true
            return
        }
    }

    // 3D vertices of a cube
    property var vertices: [
        {x: -1.0, y: -1.0, z: -1.0},
        {x:  1.0, y: -1.0, z: -1.0},
        {x:  1.0, y:  1.0, z: -1.0},
        {x: -1.0, y:  1.0, z: -1.0},
        {x: -1.0, y: -1.0, z:  1.0},
        {x:  1.0, y: -1.0, z:  1.0},
        {x:  1.0, y:  1.0, z:  1.0},
        {x: -1.0, y:  1.0, z:  1.0}
    ]

    // 12 edges connecting vertices
    property var edges: [
        {v1: 0, v2: 1}, {v1: 1, v2: 2}, {v1: 2, v2: 3}, {v1: 3, v2: 0},
        {v1: 4, v2: 5}, {v1: 5, v2: 6}, {v1: 6, v2: 7}, {v1: 7, v2: 4},
        {v1: 0, v2: 4}, {v1: 1, v2: 5}, {v1: 2, v2: 6}, {v1: 3, v2: 7}
    ]

    ListModel { id: edgesModel }
    ListModel { id: orbitModel }

    Component.onCompleted: {
        for (let i = 0; i < edges.length; i++) {
            edgesModel.append({ "x1": 0, "y1": 0, "x2": 0, "y2": 0 })
        }
        for (let i = 0; i < 20; i++) {
            orbitModel.append({ "x": 0, "y": 0 })
        }
    }

    Timer {
        id: updateTimer
        interval: 33
        running: !rootWindow.paused
        repeat: true
        onTriggered: {
            rootWindow.animTime += 0.05
            rootWindow.angleX += 0.04
            rootWindow.angleY += 0.05

            let sinX = Math.sin(rootWindow.angleX)
            let cosX = Math.cos(rootWindow.angleX)
            let sinY = Math.sin(rootWindow.angleY)
            let cosY = Math.cos(rootWindow.angleY)

            let w = cubeCanvas.pixelWidth
            let h = cubeCanvas.pixelHeight
            if (w <= 0 || h <= 0) return

            let cx = w / 2
            let cy = h / 2
            let scale = Math.min(cx, cy) * 0.55

            // 3D Projection
            let projected = []
            for (let i = 0; i < rootWindow.vertices.length; i++) {
                let v = rootWindow.vertices[i]
                let x1 = v.x * cosY - v.z * sinY
                let z1 = v.x * sinY + v.z * cosY
                let y2 = v.y * cosX - z1 * sinX
                let z2 = v.y * sinX + z1 * cosX
                let depth = 3.0 + z2 * 0.5
                let px = Math.floor(cx + (x1 * scale) / depth)
                let py = Math.floor(cy + (y2 * scale) / depth)
                projected.push({x: px, y: py})
            }

            for (let i = 0; i < rootWindow.edges.length; i++) {
                let edge = rootWindow.edges[i]
                let p1 = projected[edge.v1]
                let p2 = projected[edge.v2]
                edgesModel.setProperty(i, "x1", p1.x)
                edgesModel.setProperty(i, "y1", p1.y)
                edgesModel.setProperty(i, "x2", p2.x)
                edgesModel.setProperty(i, "y2", p2.y)
            }

            // Orbit Ring
            let orbitScaleX = cx * 0.85
            let orbitScaleY = cy * 0.4
            for (let i = 0; i < 20; i++) {
                let theta = (i / 20) * 2.0 * Math.PI + rootWindow.angleY * 0.5
                let ox = Math.floor(cx + Math.cos(theta) * orbitScaleX)
                let oy = Math.floor(cy + Math.sin(theta) * orbitScaleY + Math.cos(theta) * (cy * 0.2))
                orbitModel.setProperty(i, "x", ox)
                orbitModel.setProperty(i, "y", oy)
            }
        }
    }

    function getSinePoints(w, h, time) {
        if (w <= 0 || h <= 0) return []
        let pts = []
        let midY = h / 2
        let amp = h * 0.3
        for (let x = 0; x < w; x++) {
            let angle = (x / w) * Math.PI * 4 + time
            let y = midY + Math.sin(angle) * amp
            pts.push({ x: Math.floor(x), y: Math.floor(y) })
        }
        return pts
    }

    Column {
        anchors.fill: parent

        // Title Header
        Rectangle {
            width: parent.width
            height: 2
            color: "#1e293b"

            Text {
                anchors.fill: parent
                text: " Multi-Color Overlapping Braille Canvas Demo (Qtui)"
                color: "#f8fafc"
                bold: true
            }
        }

        // Shared Viewport & Control Panel
        Row {
            width: parent.width
            height: parent.height - 4

            // Shared Viewport Container: All 3 Canvases occupy this exact space!
            Rectangle {
                id: canvasContainer
                width: parent.width - 32
                height: parent.height
                color: "#020617"
                border.color: "#334155"

                // ----------------------------------------------------
                // Canvas 1: Cyan Phosphor 3D Wireframe Cube (#00F0FF)
                // ----------------------------------------------------
                Canvas {
                    id: cubeCanvas
                    anchors.fill: parent
                    color: "#00f0ff"
                    renderMode: Canvas.Braille
                    visible: rootWindow.showCube

                    Repeater {
                        model: orbitModel
                        delegate: Point {
                            required property var modelData
                            x: modelData.x
                            y: modelData.y
                        }
                    }

                    Repeater {
                        model: edgesModel
                        delegate: Line {
                            required property var modelData
                            x1: modelData.x1
                            y1: modelData.y1
                            x2: modelData.x2
                            y2: modelData.y2
                        }
                    }
                }

                // ----------------------------------------------------
                // Canvas 2: Green Phosphor Dynamic Sine Wave (#00FF66)
                // Shares the exact same space as Canvas 1!
                // ----------------------------------------------------
                Canvas {
                    id: sineCanvas
                    anchors.fill: parent
                    color: "#00ff66"
                    renderMode: Canvas.Braille
                    visible: rootWindow.showSine

                    Polygon {
                        closed: false
                        points: rootWindow.getSinePoints(sineCanvas.pixelWidth, sineCanvas.pixelHeight, rootWindow.animTime)
                    }
                }

                // ----------------------------------------------------
                // Canvas 3: Yellow Radar Tracker & Target (#FFB000)
                // Shares the exact same space as Canvas 1 & 2!
                // ----------------------------------------------------
                Canvas {
                    id: radarCanvas
                    anchors.fill: parent
                    color: "#ffb000"
                    renderMode: Canvas.Braille
                    visible: rootWindow.showRadar

                    property int rx: Math.floor(pixelWidth / 2 + Math.cos(rootWindow.animTime * 1.5) * (pixelWidth * 0.35))
                    property int ry: Math.floor(pixelHeight / 2 + Math.sin(rootWindow.animTime * 1.5) * (pixelHeight * 0.35))

                    Circle {
                        cx: radarCanvas.rx
                        cy: radarCanvas.ry
                        radius: Math.max(4, Math.floor(Math.min(radarCanvas.pixelWidth, radarCanvas.pixelHeight) * 0.12 + Math.sin(rootWindow.animTime * 3) * 2))
                        fill: false
                    }

                    Line {
                        x1: Math.max(0, radarCanvas.rx - 8)
                        y1: radarCanvas.ry
                        x2: Math.min(radarCanvas.pixelWidth - 1, radarCanvas.rx + 8)
                        y2: radarCanvas.ry
                    }

                    Line {
                        x1: radarCanvas.rx
                        y1: Math.max(0, radarCanvas.ry - 6)
                        x2: radarCanvas.rx
                        y2: Math.min(radarCanvas.pixelHeight - 1, radarCanvas.ry + 6)
                    }
                }
            }

            // Legend / Control Panel
            Rectangle {
                width: 31
                height: parent.height
                color: "#0f172a"
                border.color: "#334155"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1

                    Text {
                        text: "CANVAS LAYERS"
                        color: "#e2e8f0"
                        bold: true
                    }

                    Text {
                        text: (rootWindow.showCube ? "[X]" : "[ ]") + " 1: 3D Cube"
                        color: rootWindow.showCube ? "#00f0ff" : "#475569"
                    }
                    Text {
                        text: "   Color: Cyan (#00F0FF)"
                        color: "#94a3b8"
                    }

                    Text {
                        text: (rootWindow.showSine ? "[X]" : "[ ]") + " 2: Sine Wave"
                        color: rootWindow.showSine ? "#00ff66" : "#475569"
                    }
                    Text {
                        text: "   Color: Green (#00FF66)"
                        color: "#94a3b8"
                    }

                    Text {
                        text: (rootWindow.showRadar ? "[X]" : "[ ]") + " 3: Radar Tracker"
                        color: rootWindow.showRadar ? "#ffb000" : "#475569"
                    }
                    Text {
                        text: "   Color: Yellow (#FFB000)"
                        color: "#94a3b8"
                    }

                    Text {
                        text: "HOW IT WORKS"
                        color: "#e2e8f0"
                        bold: true
                    }
                    Text {
                        text: "3 separate Canvases"
                        color: "#cbd5e1"
                    }
                    Text {
                        text: "share the exact same"
                        color: "#cbd5e1"
                    }
                    Text {
                        text: "parent coordinates."
                        color: "#cbd5e1"
                    }
                    Text {
                        text: "Empty dots transparent,"
                        color: "#94a3b8"
                    }
                    Text {
                        text: "rendering multi-color"
                        color: "#94a3b8"
                    }
                    Text {
                        text: "Braille graphics!"
                        color: "#94a3b8"
                    }
                }
            }
        }

        // Footer Bar
        Rectangle {
            width: parent.width
            height: 2
            color: "#1e293b"

            Text {
                anchors.fill: parent
                text: " Controls: [1] Toggle 3D Cube | [2] Toggle Sine | [3] Toggle Radar | [Space] Pause | [Q/Esc] Quit"
                color: "#f1f5f9"
            }
        }
    }
}
