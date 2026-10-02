import QtQml 2.15
import QtQml.Models 2.15
import Terminal 1.0

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "black"
    useAlternateScreen: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    property double angleX: 0.0
    property double angleY: 0.0

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

    // 12 edges connecting the vertices
    property var edges: [
        {v1: 0, v2: 1}, {v1: 1, v2: 2}, {v1: 2, v2: 3}, {v1: 3, v2: 0}, // Back face
        {v1: 4, v2: 5}, {v1: 5, v2: 6}, {v1: 6, v2: 7}, {v1: 7, v2: 4}, // Front face
        {v1: 0, v2: 4}, {v1: 1, v2: 5}, {v1: 2, v2: 6}, {v1: 3, v2: 7}  // Connections
    ]

    ListModel {
        id: edgesModel
    }

    ListModel {
        id: orbitModel
    }

    // Initialize Models
    Component.onCompleted: {
        for (let i = 0; i < edges.length; i++) {
            edgesModel.append({ "x1": 0, "y1": 0, "x2": 0, "y2": 0 });
        }
        for (let i = 0; i < 20; i++) {
            orbitModel.append({ "x": 0, "y": 0 });
        }
        updateTimer.start();
    }

    Timer {
        id: updateTimer
        interval: 40 // ~25 FPS
        repeat: true
        onTriggered: {
            // Update rotation angles
            rootWindow.angleX += 0.05;
            rootWindow.angleY += 0.06;

            let sinX = Math.sin(rootWindow.angleX);
            let cosX = Math.cos(rootWindow.angleX);
            let sinY = Math.sin(rootWindow.angleY);
            let cosY = Math.cos(rootWindow.angleY);

            // Midpoint of canvas in sub-pixels
            let cx = (canvasFrame.width * 2) / 2;
            let cy = (canvasFrame.height * 4) / 2;
            let scale = Math.min(cx, cy) * 0.55;

            // Project 3D vertices to 2D
            let projected = [];
            for (let i = 0; i < rootWindow.vertices.length; i++) {
                let v = rootWindow.vertices[i];

                // Rotate around Y axis
                let x1 = v.x * cosY - v.z * sinY;
                let z1 = v.x * sinY + v.z * cosY;

                // Rotate around X axis
                let y2 = v.y * cosX - z1 * sinX;
                let z2 = v.y * sinX + z1 * cosX;

                // Projection with simple depth offset
                let depth = 3.0 + z2 * 0.5;
                let px = Math.floor(cx + (x1 * scale) / depth);
                let py = Math.floor(cy + (y2 * scale) / depth);
                projected.push({x: px, y: py});
            }

            // Update edge lines
            for (let i = 0; i < rootWindow.edges.length; i++) {
                let edge = rootWindow.edges[i];
                let p1 = projected[edge.v1];
                let p2 = projected[edge.v2];
                edgesModel.setProperty(i, "x1", p1.x);
                edgesModel.setProperty(i, "y1", p1.y);
                edgesModel.setProperty(i, "x2", p2.x);
                edgesModel.setProperty(i, "y2", p2.y);
            }

            // Update rotating orbit ring
            let orbitScaleX = cx * 0.85;
            let orbitScaleY = cy * 0.4;
            for (let i = 0; i < 20; i++) {
                let theta = (i / 20) * 2.0 * Math.PI + rootWindow.angleY * 0.5;
                let ox = Math.floor(cx + Math.cos(theta) * orbitScaleX);
                // Tilt the orbit vertically
                let oy = Math.floor(cy + Math.sin(theta) * orbitScaleY + Math.cos(theta) * (cy * 0.2));
                orbitModel.setProperty(i, "x", ox);
                orbitModel.setProperty(i, "y", oy);
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Header Panel
        Rectangle {
            width: parent.width
            height: 3
            color: "#1a1a1a"
            border.style: "single"
            border.color: "blue"

            Text {
                anchors.centerIn: parent
                text: "Declarative Braille Vector Graphics Canvas Demo"
                color: "yellow"
                bold: true
            }
        }

        // Subtitle instructions
        Text {
            width: parent.width
            text: "Press 'q' or 'Escape' to quit. Notice sub-pixel resolution and smooth animations!"
            color: "gray"
            horizontalAlignment: Qt.AlignHCenter
        }

        // Render Canvas Area
        Rectangle {
            id: canvasFrame
            width: parent.width
            height: parent.height - 5
            border.style: "double"
            border.color: "gray"

            BrailleCanvas {
                anchors.fill: parent
                color: "cyan"

                // Background orbit ring dots
                Repeater {
                    model: orbitModel
                    delegate: BraillePoint {
                        x: modelData.x
                        y: modelData.y
                    }
                }

                // 3D Cube lines
                Repeater {
                    model: edgesModel
                    delegate: BrailleLine {
                        x1: modelData.x1
                        y1: modelData.y1
                        x2: modelData.x2
                        y2: modelData.y2
                    }
                }
            }
        }
    }
}
