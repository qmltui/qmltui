pragma ComponentBehavior: Bound

import QtQml
import Qamui
import Qamui.Controls

Terminal {
    id: rootWindow
    color: "black"

    // Waveform Properties
    property double waveTime: 0.0
    property double frequency: 2.0
    property double noiseLevel: 0.0
    property string waveType: "sine" // sine, square, triangle, sawtooth

    // Lissajous Properties
    property double lissajousPhase: 0.0
    property int freqA: 3
    property int freqB: 4

    // App state
    property bool paused: false
    property double waveSpeed: 0.08
    property int simulatedCpu: 4

    // Explicit key handling to quit, as per project rules
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }

        // Additional shortcut keys
        if (event.key === Qt.Key_Space) {
            rootWindow.paused = !rootWindow.paused
            event.accepted = true
        }
    }

    // Mathematical calculations for the wave chart
    function generateWaveformPoints(canvasWidth, canvasHeight, time, freq, noise, type) {
        if (canvasWidth <= 0 || canvasHeight <= 0) return [];

        let pts = [];
        let w = canvasWidth * 2;
        let h = canvasHeight * 4;
        let midY = h / 2;
        let amp = h * 0.35;

        for (let x = 0; x < w; x++) {
            let angle = (x / w) * Math.PI * 2 * freq + time;
            let yVal = 0.0;

            if (type === "sine") {
                yVal = Math.sin(angle);
            } else if (type === "square") {
                yVal = Math.sin(angle) >= 0 ? 1.0 : -1.0;
            } else if (type === "triangle") {
                yVal = (Math.abs((angle % (Math.PI * 2)) - Math.PI) / Math.PI) * 2.0 - 1.0;
            } else if (type === "sawtooth") {
                yVal = ((angle % (Math.PI * 2)) / Math.PI) - 1.0;
            }

            if (noise > 0) {
                yVal += (Math.random() - 0.5) * noise;
            }

            let y = Math.round(midY - yVal * amp);
            y = Math.max(0, Math.min(h - 1, y));
            pts.push({ x: x, y: y });
        }
        return pts;
    }

    // Mathematical calculations for the Lissajous X-Y phase plot
    function generateLissajousPoints(canvasWidth, canvasHeight, phase, fA, fB) {
        if (canvasWidth <= 0 || canvasHeight <= 0) return [];

        let pts = [];
        let w = canvasWidth * 2;
        let h = canvasHeight * 4;
        let midX = w / 2;
        let midY = h / 2;
        let sizeX = w * 0.42;
        let sizeY = h * 0.42;
        let steps = 160;

        for (let i = 0; i <= steps; i++) {
            let theta = (i / steps) * Math.PI * 2;
            let xVal = Math.sin(fA * theta + phase);
            let yVal = Math.sin(fB * theta);

            let x = Math.round(midX + xVal * sizeX);
            let y = Math.round(midY - yVal * sizeY);

            x = Math.max(0, Math.min(w - 1, x));
            y = Math.max(0, Math.min(h - 1, y));
            pts.push({ x: x, y: y });
        }
        return pts;
    }

    function cycleWaveType() {
        if (waveType === "sine") waveType = "square";
        else if (waveType === "square") waveType = "triangle";
        else if (waveType === "triangle") waveType = "sawtooth";
        else waveType = "sine";
    }

    function cycleNoise() {
        if (noiseLevel === 0.0) noiseLevel = 0.15;
        else if (noiseLevel === 0.15) noiseLevel = 0.35;
        else if (noiseLevel === 0.35) noiseLevel = 0.6;
        else noiseLevel = 0.0;
    }

    function updatePlots() {
        if (waveformPlotCanvas.width > 0 && waveformPlotCanvas.height > 0) {
            waveformPoly.points = rootWindow.generateWaveformPoints(
                waveformPlotCanvas.width,
                waveformPlotCanvas.height,
                rootWindow.waveTime,
                rootWindow.frequency,
                rootWindow.noiseLevel,
                rootWindow.waveType
            );
        }
        if (lissajousPlotCanvas.width > 0 && lissajousPlotCanvas.height > 0) {
            lissajousPoly.points = rootWindow.generateLissajousPoints(
                lissajousPlotCanvas.width,
                lissajousPlotCanvas.height,
                rootWindow.lissajousPhase,
                rootWindow.freqA,
                rootWindow.freqB
            );
        }
    }

    // Animation Timer
    Timer {
        id: frameTimer
        interval: 40 // ~25 FPS
        running: true
        repeat: true
        onTriggered: {
            if (!rootWindow.paused) {
                rootWindow.waveTime += rootWindow.waveSpeed;
                rootWindow.lissajousPhase += 0.04;
                if (Math.random() < 0.1) {
                    rootWindow.simulatedCpu = Math.floor(Math.random() * 3) + 3;
                }
            }
            rootWindow.updatePlots();
        }
    }

    // Outer Layout Wrapper
    Rectangle {
        anchors.fill: parent
        color: "#111111"

        // 1. HEADER PANEL
        Rectangle {
            id: header
            height: 3
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "double"
            border.color: "cyan"
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "📊 REAL-TIME VECTOR PLOTTING DASHBOARD"
                color: "yellow"
                bold: true
            }
        }

        // 2. FOOTER PANEL
        Rectangle {
            id: footer
            height: 3
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "single"
            border.color: "cyan"
            color: "#0d1117"

            Text {
                anchors.centerIn: parent
                text: "Focus: [Tab] / [Shift+Tab] | Quick Pause: [Space] | Quit: [Q] or [Esc]"
                color: "gray"
            }
        }

        // 3. SIDEBAR CONTROLS & STATS (LEFT)
        Rectangle {
            id: sidebar
            width: 25
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: parent.left
            border.style: "single"
            border.color: "magenta"
            color: "#1e1e24"

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    width: parent.width
                    text: "📊 CONTROLS"
                    color: "magenta"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                // Play / Pause Toggle
                Button {
                    background: Rectangle {
                        color: "#21252b"
                        border.color: "default"
                        border.style: "none"
                    }
                    width: parent.width
                    height: 3
                    text: rootWindow.paused ? "▶ RESUME" : "⏸ PAUSE"
                    onClicked: rootWindow.paused = !rootWindow.paused
                }

                // Waveform type selector
                Button {
                    background: Rectangle {
                        color: "#21252b"
                        border.color: "default"
                        border.style: "none"
                    }
                    width: parent.width
                    height: 3
                    text: "WAVE: " + rootWindow.waveType.toUpperCase()
                    onClicked: rootWindow.cycleWaveType()
                }

                // Waveform frequency adjustments
                Row {
                    width: parent.width
                    height: 3
                    spacing: 1
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: Math.floor((parent.width - 1) / 2)
                        height: 3
                        text: "FREQ -"
                        onClicked: rootWindow.frequency = Math.max(0.5, rootWindow.frequency - 0.5)
                    }
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                        height: 3
                        text: "FREQ +"
                        onClicked: rootWindow.frequency = Math.min(10.0, rootWindow.frequency + 0.5)
                    }
                }

                // Noise toggles
                Button {
                    background: Rectangle {
                        color: "#21252b"
                        border.color: "default"
                        border.style: "none"
                    }
                    width: parent.width
                    height: 3
                    text: "NOISE: " + Math.round(rootWindow.noiseLevel * 100) + "%"
                    onClicked: rootWindow.cycleNoise()
                }

                // Spacer
                Text {
                    width: parent.width
                    text: "─────────────────────"
                    color: "darkgray"
                }

                Text {
                    width: parent.width
                    text: "🌀 LISSAJOUS"
                    color: "cyan"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                // Lissajous Freq A adjustments
                Row {
                    width: parent.width
                    height: 3
                    spacing: 1
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: Math.floor((parent.width - 1) / 2)
                        height: 3
                        text: "X-FREQ -"
                        onClicked: rootWindow.freqA = Math.max(1, rootWindow.freqA - 1)
                    }
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                        height: 3
                        text: "X-FREQ +"
                        onClicked: rootWindow.freqA = Math.min(12, rootWindow.freqA + 1)
                    }
                }

                // Lissajous Freq B adjustments
                Row {
                    width: parent.width
                    height: 3
                    spacing: 1
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: Math.floor((parent.width - 1) / 2)
                        height: 3
                        text: "Y-FREQ -"
                        onClicked: rootWindow.freqB = Math.max(1, rootWindow.freqB - 1)
                    }
                    Button {
                        background: Rectangle {
                            color: "#21252b"
                            border.color: "default"
                            border.style: "none"
                        }
                        width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                        height: 3
                        text: "Y-FREQ +"
                        onClicked: rootWindow.freqB = Math.min(12, rootWindow.freqB + 1)
                    }
                }

                // Spacer
                Text {
                    width: parent.width
                    text: "─────────────────────"
                    color: "darkgray"
                }

                Text {
                    width: parent.width
                    text: "📈 STATS"
                    color: "green"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: " • Freq: " + rootWindow.frequency.toFixed(1) + " Hz"
                    color: "silver"
                }

                Text {
                    width: parent.width
                    text: " • Ratio: " + rootWindow.freqA + " : " + rootWindow.freqB
                    color: "silver"
                }

                Text {
                    width: parent.width
                    text: " • Render: 25 FPS"
                    color: "silver"
                }

                Text {
                    width: parent.width
                    text: " • CPU: ~" + rootWindow.simulatedCpu + "%"
                    color: "silver"
                }
            }
        }

        // 4. MAIN PANEL (RIGHT CONTAINER)
        Rectangle {
            id: mainContent
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: sidebar.right
            anchors.right: parent.right
            color: "transparent"

            // Split into two charts stacked vertically
            Column {
                anchors.fill: parent
                spacing: 0

                // Upper Chart (Oscilloscope)
                Rectangle {
                    width: parent.width
                    height: Math.floor(parent.height / 2)
                    border.style: "double"
                    border.color: "green"
                    color: "#0a140a"

                    Text {
                        id: upperTitle
                        x: 2
                        y: 1
                        text: "OSCILLOSCOPE WAVEFORM VIEW"
                        color: "green"
                        bold: true
                    }

                    // Chart Drawing Area inset cleanly inside borders below title
                    Item {
                        id: upperChartBox
                        anchors.top: upperTitle.bottom
                        anchors.topMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.right: parent.right
                        anchors.rightMargin: 1

                        // Superposed Canvas 1: High-Performance Background Grid Lines Canvas
                        Canvas {
                            id: waveformGridCanvas
                            anchors.fill: parent
                            color: "#004400"

                            // Draw C++ native Grid lines
                            Line {
                                x1: 0
                                y1: Math.floor(waveformGridCanvas.height * 2)
                                x2: Math.floor(waveformGridCanvas.width * 2)
                                y2: Math.floor(waveformGridCanvas.height * 2)
                            }

                            Repeater {
                                model: 5
                                delegate: Line {
                                    required property int index
                                    x1: Math.floor((index + 1) * (waveformGridCanvas.width * 2) / 6)
                                    y1: 0
                                    x2: Math.floor((index + 1) * (waveformGridCanvas.width * 2) / 6)
                                    y2: Math.floor(waveformGridCanvas.height * 4)
                                }
                            }

                            Repeater {
                                model: 3
                                delegate: Line {
                                    required property int index
                                    x1: 0
                                    y1: Math.floor((index + 1) * (waveformGridCanvas.height * 4) / 4)
                                    x2: Math.floor(waveformGridCanvas.width * 2)
                                    y2: Math.floor((index + 1) * (waveformGridCanvas.height * 4) / 4)
                                }
                            }
                        }

                        // Superposed Canvas 2: High-Performance Foreground Waveform Plot Canvas
                        Canvas {
                            id: waveformPlotCanvas
                            anchors.fill: parent
                            color: "green"

                            onWidthChanged: rootWindow.updatePlots()
                            onHeightChanged: rootWindow.updatePlots()

                            // Dynamic vector curve
                            Polygon {
                                id: waveformPoly
                                closed: false
                            }
                        }
                    }
                }

                // Lower Chart (Phase XY / Lissajous Orbit)
                Rectangle {
                    width: parent.width
                    height: parent.height - Math.floor(parent.height / 2)
                    border.style: "double"
                    border.color: "cyan"
                    color: "#0a14a"

                    Text {
                        id: lowerTitle
                        x: 2
                        y: 1
                        text: "XY PHASE ORBIT PLOT"
                        color: "cyan"
                        bold: true
                    }

                    // Chart Drawing Area inset cleanly inside borders below title
                    Item {
                        id: lowerChartBox
                        anchors.top: lowerTitle.bottom
                        anchors.topMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.right: parent.right
                        anchors.rightMargin: 1

                        // Superposed Canvas 1: High-Performance Background Cross-hairs Grid Canvas
                        Canvas {
                            id: lissajousGridCanvas
                            anchors.fill: parent
                            color: "#004444"

                            Line {
                                x1: 0
                                y1: Math.floor(lissajousGridCanvas.height * 2)
                                x2: Math.floor(lissajousGridCanvas.width * 2)
                                y2: Math.floor(lissajousGridCanvas.height * 2)
                            }

                            Line {
                                x1: Math.floor(lissajousGridCanvas.width)
                                y1: 0
                                x2: Math.floor(lissajousGridCanvas.width)
                                y2: Math.floor(lissajousGridCanvas.height * 4)
                            }
                        }

                        // Superposed Canvas 2: High-Performance Foreground Lissajous Orbit Plot Canvas
                        Canvas {
                            id: lissajousPlotCanvas
                            anchors.fill: parent
                            color: "cyan"

                            onWidthChanged: rootWindow.updatePlots()
                            onHeightChanged: rootWindow.updatePlots()

                            // Dynamic Lissajous curve
                            Polygon {
                                id: lissajousPoly
                                closed: true
                            }
                        }
                    }
                }
            }
        }
    }
}
