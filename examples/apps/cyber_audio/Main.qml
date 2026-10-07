pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    title: "// CYBERPUNK 2077 // NEURAL AUDIO MATRIX"
    color: "#05050a"

    // ==========================================
    // KEYBOARD INPUT & QUIT PATH (AGENTS.md)
    // ==========================================
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Space) {
            root.isPlaying = !root.isPlaying
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_1) {
            root.state = "split"
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_2) {
            root.state = "spectrum"
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_3) {
            root.state = "scope"
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_B) {
            root.bassBoost = !root.bassBoost
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_G) {
            root.state = (root.state === "glitch" ? "split" : "glitch")
            event.accepted = true
            return
        }
    }

    // ==========================================
    // APPLICATION STATE & MOTION PROPERTIES
    // ==========================================
    property bool isPlaying: true
    property bool bassBoost: false
    property string activePreset: "SYNTHWAVE RUNNER"
    property int bpm: 138

    // Dynamic accent color with Behavior cross-fade
    property string accentColor: "#00f0ff"
    Behavior on accentColor {
        ColorAnimation { duration: 250 }
    }

    property string secondaryColor: "#ff0055"
    Behavior on secondaryColor {
        ColorAnimation { duration: 250 }
    }

    // Beat pulse scalar for glowing beat indicator
    property real beatPulse: 0.0
    property int beatIndex: 0

    // Stereo VU meter levels with analog ballistic damping
    property real vuLeft: 0.0
    Behavior on vuLeft {
        NumberAnimation { duration: 90; easing: Easing.OutQuad }
    }

    property real vuRight: 0.0
    Behavior on vuRight {
        NumberAnimation { duration: 90; easing: Easing.OutQuad }
    }

    // Waveform phase animation
    property real wavePhase: 0.0

    // 24-Band Frequency Data
    readonly property var freqLabels: [
        "40", "60", "80", "120", "180", "250", "350", "500",
        "700", "1k", "1.4k", "2k", "2.8k", "4k", "5.6k", "8k",
        "10k", "11k", "12k", "13k", "14k", "15k", "16k", "18k"
    ]
    property var bandLevels: [
        0.1, 0.2, 0.3, 0.4, 0.5, 0.4, 0.3, 0.2,
        0.3, 0.5, 0.6, 0.7, 0.5, 0.4, 0.3, 0.2,
        0.2, 0.3, 0.4, 0.3, 0.2, 0.1, 0.1, 0.1
    ]
    property var peakLevels: [
        0.2, 0.3, 0.4, 0.5, 0.6, 0.5, 0.4, 0.3,
        0.4, 0.6, 0.7, 0.8, 0.6, 0.5, 0.4, 0.3,
        0.3, 0.4, 0.5, 0.4, 0.3, 0.2, 0.2, 0.1
    ]

    // Block character resolution table
    readonly property var blockGlyphs: [" ", " ", "▂", "▃", "▄", "▅", "▆", "▇", "█"]

    // ==========================================
    // PROCEDURAL SYNTHWAVE AUDIO SIMULATION
    // ==========================================
    property real synthTime: 0.0

    Timer {
        id: audioEngineTimer
        interval: 33 // ~30-60 Hz refresh for audio synthesis
        repeat: true
        running: root.isPlaying
        onTriggered: root.stepAudio()
    }

    function stepAudio() {
        synthTime += 0.06;
        wavePhase = (wavePhase + 0.18) % 6.28318;

        // 138 BPM beat tracking (every beat = ~434 ms -> ~13.1 steps at 33ms)
        let beatStep = Math.floor(synthTime * 10) % 4;
        if (beatStep !== beatIndex) {
            beatIndex = beatStep;
            beatPulse = 1.0;
        } else {
            beatPulse = Math.max(0.0, beatPulse - 0.12);
        }

        let isKick = (beatIndex === 0 || beatIndex === 2);
        let isSnare = (beatIndex === 1 || beatIndex === 3);

        let newBands = [];
        let newPeaks = [];

        let bassMultiplier = root.bassBoost ? 1.55 : 1.0;
        let isGlitch = (root.state === "glitch");

        for (let i = 0; i < 24; ++i) {
            let base = 0.15;

            // Sub-bass & Bass frequencies (Bands 0 - 5)
            if (i < 6) {
                let kickEnergy = isKick ? (0.75 - i * 0.08) * beatPulse : 0.2;
                let bassLine = Math.abs(Math.sin(synthTime * 3.5 + i * 0.4)) * 0.45;
                base = (kickEnergy + bassLine) * bassMultiplier;
            }
            // Low-Mid & Mid (Bands 6 - 13: Snare & Synth Melodic Arp)
            else if (i < 14) {
                let snareEnergy = isSnare ? 0.65 * beatPulse : 0.15;
                let arpPhase = Math.sin(synthTime * 6.0 + i * 0.8);
                let synthArp = arpPhase > 0.3 ? arpPhase * 0.55 : 0.1;
                base = snareEnergy + synthArp;
            }
            // High-Mid & Treble (Bands 14 - 23: Hi-Hats, Noise & Air)
            else {
                let hiHat = Math.abs(Math.cos(synthTime * 12.0 + i)) * 0.4;
                let air = Math.abs(Math.sin(synthTime * 2.0 + i * 0.5)) * 0.25;
                base = hiHat + air;
            }

            // Glitch overdrive chaotic distortion
            if (isGlitch) {
                base += (Math.random() - 0.5) * 0.4;
            }

            // Clamp 0.0 .. 1.0
            let targetVal = Math.max(0.05, Math.min(1.0, base));

            // Smooth band decay
            let prevVal = root.bandLevels[i] || 0.1;
            let finalVal = Math.max(targetVal, prevVal * 0.72);
            newBands.push(finalVal);

            // Peak hold decay with gravity
            let prevPeak = root.peakLevels[i] || 0.1;
            let finalPeak = prevPeak;
            if (finalVal >= prevPeak) {
                finalPeak = finalVal;
            } else {
                finalPeak = Math.max(0.05, prevPeak - 0.025); // slow drop
            }
            newPeaks.push(finalPeak);
        }

        root.bandLevels = newBands;
        root.peakLevels = newPeaks;

        // Drive VU meters
        let leftEnergy = (newBands[1] * 0.55 + newBands[8] * 0.35 + newBands[16] * 0.1);
        let rightEnergy = (newBands[2] * 0.50 + newBands[10] * 0.35 + newBands[18] * 0.15);
        if (isGlitch) {
            leftEnergy = Math.min(1.0, leftEnergy * 1.3);
            rightEnergy = Math.min(1.0, rightEnergy * 1.3);
        }
        root.vuLeft = Math.max(0.05, Math.min(1.0, leftEnergy));
        root.vuRight = Math.max(0.05, Math.min(1.0, rightEnergy));

        // Redraw Braille oscilloscope
        root.drawOscilloscope();
    }

    // High-resolution Braille oscilloscope rendering
    function drawOscilloscope() {
        if (!scopeCanvas) return;
        scopeCanvas.clear();

        let pw = scopeCanvas.pixelWidth;
        let ph = scopeCanvas.pixelHeight;
        if (pw <= 0 || ph <= 0) return;

        let midY = ph / 2;
        let amp = ph * 0.38;

        if (root.state === "scope") {
            // Full Lissajous / Cyber Phase scope mode
            let midX = pw / 2;
            let sizeX = pw * 0.42;
            let sizeY = ph * 0.42;
            let steps = 120;
            for (let s = 0; s <= steps; s++) {
                let theta = (s / steps) * Math.PI * 2;
                let xVal = Math.sin(3 * theta + root.wavePhase);
                let yVal = Math.sin(4 * theta);
                let px = Math.round(midX + xVal * sizeX);
                let py = Math.round(midY - yVal * sizeY);
                if (px >= 0 && px < pw && py >= 0 && py < ph) {
                    scopeCanvas.setPixel(px, py, true);
                }
            }
        } else {
            // Stereo synthwave audio waveform mode
            for (let x = 0; x < pw; x++) {
                let normX = (x / pw) * Math.PI * 4;
                let wave1 = Math.sin(normX + root.wavePhase);
                let wave2 = Math.sin(normX * 2.2 - root.wavePhase * 1.4) * 0.4;
                let wave3 = (root.state === "glitch") ? (Math.random() - 0.5) * 0.5 : 0.0;
                let combined = wave1 + wave2 + wave3;
                let py = Math.round(midY - combined * amp);
                if (py >= 0 && py < ph) {
                    scopeCanvas.setPixel(x, py, true);
                }
            }
        }
        scopeCanvas.requestPaint();
    }

    // Helper: decibels from normalized 0..1 value
    function toDecibels(val) {
        if (val <= 0.01) return "-∞ dB";
        let db = 20 * Math.log10(val);
        return (db >= 0 ? "+" : "") + db.toFixed(1) + " dB";
    }

    // ==========================================
    // UI STATES & TRANSITIONS (LAYOUT MORPHING)
    // ==========================================
    state: "split"

    states: [
        State {
            name: "split"
            PropertyChanges { target: root; properties: { "accentColor": "#00f0ff", "secondaryColor": "#ff0055" } }
            PropertyChanges { target: spectrumBox; properties: { "height": 12 } }
            PropertyChanges { target: scopeBox; properties: { "height": 10 } }
        },
        State {
            name: "spectrum"
            PropertyChanges { target: root; properties: { "accentColor": "#39ff14", "secondaryColor": "#00f0ff" } }
            PropertyChanges { target: spectrumBox; properties: { "height": 18 } }
            PropertyChanges { target: scopeBox; properties: { "height": 4 } }
        },
        State {
            name: "scope"
            PropertyChanges { target: root; properties: { "accentColor": "#9d00ff", "secondaryColor": "#00f0ff" } }
            PropertyChanges { target: spectrumBox; properties: { "height": 6 } }
            PropertyChanges { target: scopeBox; properties: { "height": 16 } }
        },
        State {
            name: "glitch"
            PropertyChanges { target: root; properties: { "accentColor": "#ff003c", "secondaryColor": "#ffb700" } }
            PropertyChanges { target: spectrumBox; properties: { "height": 12 } }
            PropertyChanges { target: scopeBox; properties: { "height": 10 } }
        }
    ]

    transitions: [
        Transition {
            // Morph panel heights with satisfying bounce!
            NumberAnimation {
                target: spectrumBox
                property: "height"
                duration: 380
                easing: Easing.OutBounce
            }
            NumberAnimation {
                target: scopeBox
                property: "height"
                duration: 380
                easing: Easing.OutBounce
            }
        }
    ]

    // ==========================================
    // VISUAL SHELL & PANELS
    // ==========================================
    Rectangle {
        id: container
        anchors.fill: parent
        color: "#05050a"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // -------------------------------------------------------------
            // 1. TOP STATUS DECK
            // -------------------------------------------------------------
            Rectangle {
                id: topDeck
                width: parent.width
                height: 3
                color: "#0a0a14"
                border.style: "double"
                border.color: root.accentColor

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 2
                    anchors.rightMargin: 2
                    spacing: 2

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "⚡ NEURAL AUDIO MATRIX"
                        color: root.accentColor
                        bold: true
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "│ " + root.activePreset + " │ 138 BPM"
                        color: "#94a3b8"
                    }

                    Item { width: 4; height: 1 }

                    // Pulsing Beat LED
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10
                        height: 1
                        color: root.beatPulse > 0.5 ? root.secondaryColor : "#1e293b"
                        Text {
                            anchors.centerIn: parent
                            text: root.beatPulse > 0.5 ? "● BEAT" : "○ beat"
                            color: root.beatPulse > 0.5 ? "#ffffff" : "#64748b"
                            bold: true
                        }
                    }

                    Item { width: 2; height: 1 }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.isPlaying ? "▶ 60 FPS ACTIVE" : "⏸ PAUSED"
                        color: root.isPlaying ? "#22c55e" : "#eab308"
                        bold: true
                    }
                }
            }

            // -------------------------------------------------------------
            // 2. SPECTRUM ANALYZER PANEL (24 BANDS)
            // -------------------------------------------------------------
            Rectangle {
                id: spectrumBox
                width: parent.width
                height: 12
                color: "#080812"
                border.style: "single"
                border.color: root.accentColor

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    // Panel Header Line
                    Row {
                        width: parent.width
                        height: 1
                        spacing: 2
                        Text {
                            text: " 24-BAND NEON EQUALIZER"
                            color: root.accentColor
                            bold: true
                        }
                        Text {
                            text: root.bassBoost ? " [BASS BOOST ON]" : ""
                            color: "#ff0055"
                            bold: true
                        }
                    }

                    // 24 Equalizer Columns
                    Row {
                        id: equalizerRow
                        width: parent.width - 2
                        height: spectrumBox.height - 3
                        spacing: Math.max(1, Math.floor((width - (24 * 2)) / 23))

                        Repeater {
                            model: 24
                            delegate: Item {
                                id: bandCol
                                required property int index
                                width: 2
                                height: parent.height

                                readonly property real levelVal: root.bandLevels[index] || 0.0
                                readonly property real peakVal: root.peakLevels[index] || 0.0

                                // Calculate filled blocks height in rows
                                readonly property int totalRows: Math.max(1, height - 1)
                                readonly property int filledRows: Math.min(totalRows, Math.round(levelVal * totalRows))
                                readonly property int peakRow: Math.min(totalRows, Math.round(peakVal * totalRows))

                                Column {
                                    anchors.fill: parent
                                    spacing: 0

                                    // Bar area
                                    Column {
                                        width: 2
                                        height: bandCol.totalRows
                                        spacing: 0

                                        Repeater {
                                            model: bandCol.totalRows
                                            delegate: Text {
                                                required property int index
                                                readonly property int rowFromBottom: bandCol.totalRows - index
                                                text: {
                                                    if (rowFromBottom === bandCol.peakRow) return "▔▔";
                                                    if (rowFromBottom <= bandCol.filledRows) return "██";
                                                    return "  ";
                                                }
                                                color: {
                                                    if (rowFromBottom === bandCol.peakRow) return "#ffffff";
                                                    let ratio = rowFromBottom / bandCol.totalRows;
                                                    if (ratio > 0.75) return root.secondaryColor;
                                                    if (ratio > 0.45) return "#9d00ff";
                                                    return root.accentColor;
                                                }
                                                bold: true
                                            }
                                        }
                                    }

                                    // Frequency Label strictly at bottom row
                                    Text {
                                        width: 2
                                        height: 1
                                        text: root.freqLabels[bandCol.index].substring(0, 2)
                                        color: "#64748b"
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -------------------------------------------------------------
            // 3. LOWER SECTION: STEREO VU METERS & BRAILLE OSCILLOSCOPE
            // -------------------------------------------------------------
            Rectangle {
                id: scopeBox
                width: parent.width
                height: 10
                color: "#080812"
                border.style: "single"
                border.color: root.secondaryColor

                Row {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 2

                    // --- VU METERS (LEFT DECK) ---
                    Column {
                        width: 32
                        height: parent.height
                        spacing: 1

                        Text {
                            text: "STEREO PEAK VU METERS"
                            color: root.secondaryColor
                            bold: true
                        }

                        // Left Channel VU Meter
                        Column {
                            spacing: 0
                            Text {
                                text: "CH-L " + root.toDecibels(root.vuLeft)
                                color: "#cbd5e1"
                            }
                            ProgressBar {
                                width: 30
                                showBrackets: true
                                value: root.vuLeft * 100
                                from: 0
                                to: 100
                                color: root.vuLeft > 0.85 ? "#ef4444" : (root.vuLeft > 0.6 ? "#eab308" : root.accentColor)
                            }
                        }

                        // Right Channel VU Meter
                        Column {
                            spacing: 0
                            Text {
                                text: "CH-R " + root.toDecibels(root.vuRight)
                                color: "#cbd5e1"
                            }
                            ProgressBar {
                                width: 30
                                showBrackets: true
                                value: root.vuRight * 100
                                from: 0
                                to: 100
                                color: root.vuRight > 0.85 ? "#ef4444" : (root.vuRight > 0.6 ? "#eab308" : root.accentColor)
                            }
                        }
                    }

                    // --- VERTICAL DIVIDER ---
                    Rectangle {
                        width: 1
                        height: parent.height
                        color: "#1e293b"
                    }

                    // --- BRAILLE OSCILLOSCOPE (RIGHT DECK) ---
                    Column {
                        width: parent.width - 36
                        height: parent.height
                        spacing: 0

                        Row {
                            width: parent.width
                            height: 1
                            spacing: 1
                            Text {
                                text: (root.state === "scope") ? "LISSAJOUS 3D PHASE SCOPE" : "BRAILLE REAL-TIME OSCILLOSCOPE"
                                color: root.accentColor
                                bold: true
                            }
                            Text {
                                text: " [2x4 subpixels/cell]"
                                color: "#64748b"
                            }
                        }

                        // Braille Canvas Drawing Area
                        Canvas {
                            id: scopeCanvas
                            width: parent.width
                            height: Math.max(1, parent.height - 1)
                            renderMode: Canvas.RenderMode.Braille
                            color: root.accentColor
                        }
                    }
                }
            }

            // -------------------------------------------------------------
            // 4. BOTTOM INTERACTION / SHORTCUT FOOTER
            // -------------------------------------------------------------
            Rectangle {
                id: footer
                width: parent.width
                height: 2
                color: "#0a0a14"
                border.style: "single"
                border.color: "#334155"

                Row {
                    anchors.centerIn: parent
                    spacing: 3

                    Text {
                        text: "[Space] Pause/Play"
                        color: root.isPlaying ? "#38bdf8" : "#f59e0b"
                        bold: true
                    }
                    Text { text: "•"; color: "#475569" }
                    Text {
                        text: "[1] Split"
                        color: root.state === "split" ? root.accentColor : "#94a3b8"
                        bold: root.state === "split"
                    }
                    Text {
                        text: "[2] Spectrum"
                        color: root.state === "spectrum" ? root.accentColor : "#94a3b8"
                        bold: root.state === "spectrum"
                    }
                    Text {
                        text: "[3] Scope"
                        color: root.state === "scope" ? root.accentColor : "#94a3b8"
                        bold: root.state === "scope"
                    }
                    Text { text: "•"; color: "#475569" }
                    Text {
                        text: "[B] Bass (" + (root.bassBoost ? "ON" : "OFF") + ")"
                        color: root.bassBoost ? "#ff0055" : "#94a3b8"
                        bold: root.bassBoost
                    }
                    Text {
                        text: "[G] Glitch"
                        color: root.state === "glitch" ? "#ff003c" : "#94a3b8"
                        bold: root.state === "glitch"
                    }
                    Text { text: "•"; color: "#475569" }
                    Text {
                        text: "[Q / Esc] Quit"
                        color: "#64748b"
                    }
                }
            }
        }
    }
}
