pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    title: "// CYBERPUNK 2077 // NEURAL AUDIO MATRIX"
    color: "#05050a"
    focus: true

    // ==========================================
    // KEYBOARD INPUT & QUIT PATH (AGENTS.md)
    // ==========================================
    // Layout: 1 / 2 / 3.  Effects: B / G.  Transport: Space.  Quit: Q / Esc.
    // Printable keys arrive as text; Escape and Space are matched by key code too.
    Keys.onPressed: (event) => {
        let token = event.text ? event.text.toLowerCase() : ""
        if (event.key === Qt.Key_Escape)
            token = "q"
        else if (event.key === Qt.Key_Space)
            token = " "

        switch (token) {
        case "q":
            Qt.quit()
            break
        case " ":
            root.isPlaying = !root.isPlaying
            break
        case "1":
            root.state = "split"
            break
        case "2":
            root.state = "spectrum"
            break
        case "3":
            root.state = "scope"
            break
        case "b":
            root.bassBoost = !root.bassBoost
            break
        case "g":
            root.glitch = !root.glitch
            break
        default:
            return
        }
        event.accepted = true
    }

    // ==========================================
    // APPLICATION STATE
    // ==========================================
    property bool isPlaying: true
    property bool bassBoost: false
    property bool glitch: false
    readonly property string activePreset: "SYNTHWAVE RUNNER"
    readonly property int bpm: 138
    readonly property int bandCount: 24

    // Share of the main area given to the equalizer panel (0..1).
    // Changed by the layout states below and animated by the transition.
    property real spectrumShare: 0.55

    readonly property string modeName: root.state === "spectrum" ? "SPECTRUM"
                                     : root.state === "scope" ? "SCOPE" : "SPLIT"

    // Neon palette follows the layout mode; glitch overrides it with alert colors.
    readonly property string modeAccent: root.state === "spectrum" ? "#39ff14"
                                       : root.state === "scope" ? "#c084fc" : "#00f0ff"
    property string accentColor: root.glitch ? "#ff003c" : root.modeAccent
    Behavior on accentColor {
        ColorAnimation { duration: 250 }
    }
    property string secondaryColor: root.glitch ? "#ffb700"
                                  : root.state === "split" ? "#ff0055" : "#00f0ff"
    Behavior on secondaryColor {
        ColorAnimation { duration: 250 }
    }
    readonly property string midColor: "#a855f7"
    readonly property string dimColor: "#475569"
    readonly property string mutedColor: "#64748b"
    readonly property string panelColor: "#080b14"
    readonly property string frameColor: "#1e293b"

    // Beat clock: 138 BPM = one beat every ~434 ms = 13 engine ticks at 33 ms.
    property int tick: 0
    property int beatIndex: 0
    property real beatPulse: 0.0

    // Stereo VU levels with procedural ballistics (fast attack, slow release).
    property real vuLeft: 0.18
    property real vuRight: 0.16

    // Oscilloscope phase.
    property real wavePhase: 0.0

    // 24-band spectrum data. Labels are at most 3 cells so they fit under narrow bars.
    readonly property var freqLabels: [
        "40", "60", "80", "120", "180", "250", "350", "500",
        "700", "1k", "1k4", "2k", "2k8", "4k", "5k6", "8k",
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

    // 1/8-cell vertical resolution for smooth bar tops.
    readonly property var blockGlyphs: [" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"]

    // ==========================================
    // PROCEDURAL SYNTHWAVE AUDIO SIMULATION
    // ==========================================
    property real synthTime: 0.0

    Timer {
        id: audioEngineTimer
        interval: 33
        repeat: true
        running: root.isPlaying
        onTriggered: root.stepAudio()
    }

    function stepAudio() {
        tick += 1;
        synthTime += 0.06;
        wavePhase = (wavePhase + 0.18) % 6.28318;

        let beatStep = Math.floor(tick / 13) % 4;
        if (beatStep !== beatIndex) {
            beatIndex = beatStep;
            beatPulse = 1.0;
        } else {
            beatPulse = Math.max(0.0, beatPulse - 0.12);
        }

        let isKick = (beatIndex === 0 || beatIndex === 2);
        let isSnare = (beatIndex === 1 || beatIndex === 3);
        let bassMultiplier = root.bassBoost ? 1.55 : 1.0;

        let newBands = [];
        let newPeaks = [];

        for (let i = 0; i < root.bandCount; ++i) {
            let base = 0.15;

            if (i < 6) {
                // Sub-bass & bass: kick drum plus bass line
                let kickEnergy = isKick ? (0.75 - i * 0.08) * beatPulse : 0.2;
                let bassLine = Math.abs(Math.sin(synthTime * 3.5 + i * 0.4)) * 0.45;
                base = (kickEnergy + bassLine) * bassMultiplier;
            } else if (i < 14) {
                // Low-mid & mid: snare plus melodic arp
                let snareEnergy = isSnare ? 0.65 * beatPulse : 0.15;
                let arpPhase = Math.sin(synthTime * 6.0 + i * 0.8);
                let synthArp = arpPhase > 0.3 ? arpPhase * 0.55 : 0.1;
                base = snareEnergy + synthArp;
            } else {
                // High-mid & treble: hi-hats, noise and air
                let hiHat = Math.abs(Math.cos(synthTime * 12.0 + i)) * 0.4;
                let air = Math.abs(Math.sin(synthTime * 2.0 + i * 0.5)) * 0.25;
                base = hiHat + air;
            }

            if (root.glitch) {
                base += (Math.random() - 0.5) * 0.4;
            }

            let target = Math.max(0.03, Math.min(1.0, base));

            // Exponential smoothing: quick attack, slower release, no hard snaps.
            let prev = root.bandLevels[i];
            let rate = target > prev ? 0.6 : 0.2;
            let level = prev + (target - prev) * rate;
            newBands.push(level);

            // Peak hold with gravity.
            let prevPeak = root.peakLevels[i];
            newPeaks.push(level >= prevPeak ? level : Math.max(level, prevPeak - 0.02));
        }

        root.bandLevels = newBands;
        root.peakLevels = newPeaks;

        let leftEnergy = newBands[1] * 0.55 + newBands[8] * 0.35 + newBands[16] * 0.1;
        let rightEnergy = newBands[2] * 0.50 + newBands[10] * 0.35 + newBands[18] * 0.15;
        if (root.glitch) {
            leftEnergy *= 1.3;
            rightEnergy *= 1.3;
        }
        let targetL = Math.max(0.04, Math.min(1.0, leftEnergy));
        let targetR = Math.max(0.04, Math.min(1.0, rightEnergy));
        root.vuLeft = targetL > root.vuLeft
            ? (root.vuLeft * 0.35 + targetL * 0.65)
            : (root.vuLeft * 0.88 + targetL * 0.12);
        root.vuRight = targetR > root.vuRight
            ? (root.vuRight * 0.35 + targetR * 0.65)
            : (root.vuRight * 0.88 + targetR * 0.12);

        root.drawOscilloscope();
    }

    // ==========================================
    // EQUALIZER TEXT RENDERING (one Text per row)
    // ==========================================
    // Builds the string for one row of the equalizer. `row` is 0 at the bottom.
    function spectrumRow(row, rows, barWidth, gap, pad) {
        let line = " ".repeat(pad);
        let sep = " ".repeat(gap);
        for (let i = 0; i < root.bandCount; ++i) {
            let fill = root.bandLevels[i] * rows - row;
            let glyph;
            if (fill >= 1.0) {
                glyph = "█";
            } else if (fill > 0.0) {
                glyph = root.blockGlyphs[Math.round(fill * 8)];
            } else {
                let peakRow = Math.min(rows - 1, Math.floor(root.peakLevels[i] * rows));
                glyph = (peakRow === row) ? "▔" : " ";
            }
            line += glyph.repeat(barWidth);
            if (i < root.bandCount - 1)
                line += sep;
        }
        return line;
    }

    // Frequency labels aligned under each bar. Narrow pitches show every other label.
    function labelRow(barWidth, gap, pad, maxWidth) {
        let pitch = barWidth + gap;
        let showAll = pitch >= 4;
        let line = " ".repeat(pad);
        for (let i = 0; i < root.bandCount; ++i) {
            let label = (showAll || i % 2 === 0) ? root.freqLabels[i] : "";
            line += label.padEnd(pitch, " ");
        }
        return line.substring(0, maxWidth);
    }

    // ==========================================
    // BRAILLE OSCILLOSCOPE RENDERING
    // ==========================================
    function drawCanvasLine(x0, y0, x1, y1, pw, ph) {
        let dx = Math.abs(x1 - x0);
        let dy = Math.abs(y1 - y0);
        let sx = (x0 < x1) ? 1 : -1;
        let sy = (y0 < y1) ? 1 : -1;
        let err = dx - dy;
        while (true) {
            if (x0 >= 0 && x0 < pw && y0 >= 0 && y0 < ph) {
                scopeCanvas.setPixel(x0, y0, true);
            }
            if (x0 === x1 && y0 === y1) break;
            let e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }

    function drawOscilloscope() {
        if (!scopeCanvas) return;
        scopeCanvas.clear();

        let pw = scopeCanvas.pixelWidth;
        let ph = scopeCanvas.pixelHeight;
        if (pw <= 0 || ph <= 0) return;

        let midY = ph / 2;

        if (root.state === "scope") {
            // Lissajous phase scope
            let midX = pw / 2;
            let sizeX = pw * 0.44;
            let sizeY = ph * 0.44;
            let steps = 180;
            let prevX = -1;
            let prevY = -1;
            for (let s = 0; s <= steps; s++) {
                let theta = (s / steps) * Math.PI * 2;
                let px = Math.round(midX + Math.sin(3 * theta + root.wavePhase) * sizeX);
                let py = Math.round(midY - Math.sin(4 * theta) * sizeY);
                if (s > 0) {
                    root.drawCanvasLine(prevX, prevY, px, py, pw, ph);
                }
                prevX = px;
                prevY = py;
            }
        } else {
            // Synthwave waveform
            let amp = ph * 0.38;
            let prevX = 0;
            let prevY = Math.round(midY);
            for (let x = 0; x < pw; x++) {
                let normX = (x / pw) * Math.PI * 4;
                let wave1 = Math.sin(normX + root.wavePhase);
                let wave2 = Math.sin(normX * 2.2 - root.wavePhase * 1.4) * 0.4;
                let wave3 = root.glitch ? (Math.random() - 0.5) * 0.5 : 0.0;
                let py = Math.round(midY - (wave1 + wave2 + wave3) * amp);
                if (x === 0) {
                    prevY = py;
                }
                root.drawCanvasLine(prevX, prevY, x, py, pw, ph);
                prevX = x;
                prevY = py;
            }
        }
        scopeCanvas.requestPaint();
    }

    function toDecibels(val) {
        if (val <= 0.01) return "-∞ dB";
        let db = 20 * Math.log10(val);
        return (db >= 0 ? "+" : "") + db.toFixed(1) + " dB";
    }

    // ==========================================
    // LAYOUT STATES & TRANSITIONS
    // ==========================================
    state: "split"

    states: [
        State {
            name: "split"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.55 } }
        },
        State {
            name: "spectrum"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.78 } }
        },
        State {
            name: "scope"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.28 } }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                target: root
                property: "spectrumShare"
                duration: 380
                easing: Easing.OutBounce
            }
        }
    ]

    // ==========================================
    // REUSABLE FOOTER KEY HINT
    // ==========================================
    component KeyHint: Row {
        id: hint
        property string key: ""
        property string label: ""
        property bool active: false
        property string activeColor: root.accentColor
        spacing: 1

        Text {
            text: " " + hint.key + " "
            reversed: true
            bold: true
            color: hint.active ? hint.activeColor : "#94a3b8"
        }
        Text {
            text: hint.label
            bold: hint.active
            color: hint.active ? hint.activeColor : root.mutedColor
        }
    }

    // ==========================================
    // VISUAL SHELL
    // ==========================================

    // --- 1. HEADER (1 row) ---
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#0c101c"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 1
            spacing: 2

            Text {
                text: "⚡ NEURAL AUDIO MATRIX"
                color: root.accentColor
                bold: true
            }
            Text {
                visible: root.width >= 100
                text: "│ " + root.activePreset + " │ " + root.bpm + " BPM"
                color: root.mutedColor
            }
            Text {
                text: "│ MODE " + root.modeName
                color: root.mutedColor
            }
            Text {
                visible: root.bassBoost
                text: " BASS+ "
                reversed: true
                bold: true
                color: root.secondaryColor
            }
            Text {
                visible: root.glitch
                text: " GLITCH "
                reversed: true
                bold: true
                color: "#ff003c"
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 1
            spacing: 2

            Text {
                text: root.beatPulse > 0.5 ? "● BEAT" : "○ beat"
                color: root.beatPulse > 0.5 ? root.secondaryColor : root.dimColor
                bold: root.beatPulse > 0.5
            }
            Text {
                text: root.isPlaying ? "▶ PLAYING" : "⏸ PAUSED"
                color: root.isPlaying ? "#22c55e" : "#eab308"
                bold: true
            }
        }
    }

    // --- 2. FOOTER: KEY MAP (1 row) ---
    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#0c101c"

        Row {
            anchors.centerIn: parent
            spacing: root.width >= 100 ? 3 : 1

            KeyHint {
                key: "SPACE"
                label: root.isPlaying ? "Pause" : "Play"
                active: !root.isPlaying
                activeColor: "#eab308"
            }
            KeyHint { key: "1"; label: "Split"; active: root.state === "split" }
            KeyHint { key: "2"; label: "Spectrum"; active: root.state === "spectrum" }
            KeyHint { key: "3"; label: "Scope"; active: root.state === "scope" }
            KeyHint {
                key: "B"
                label: "Bass"
                active: root.bassBoost
                activeColor: root.secondaryColor
            }
            KeyHint {
                key: "G"
                label: "Glitch"
                active: root.glitch
                activeColor: "#ff003c"
            }
            KeyHint { key: "Q"; label: "Quit" }
        }
    }

    // --- 3. MAIN AREA: two panels sharing the full height ---
    Column {
        id: mainArea
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        // Keep both panels tall enough for their content, then split by share.
        readonly property int minSpectrumHeight: 6
        readonly property int minScopeHeight: 7
        readonly property int spectrumHeight: Math.max(minSpectrumHeight,
            Math.min(height - minScopeHeight, Math.round(height * root.spectrumShare)))

        // ---- 24-BAND EQUALIZER ----
        Panel {
            id: spectrumPanel
            width: parent.width
            height: mainArea.spectrumHeight
            color: root.panelColor
            title: "24-BAND NEON EQUALIZER"
            titleColor: root.state === "spectrum" ? root.accentColor : "#38bdf8"
            border.color: root.state === "spectrum" ? root.accentColor : root.frameColor

            Item {
                id: eqArea
                anchors.fill: parent
                anchors.topMargin: 2       // border row + one blank row below the title
                anchors.bottomMargin: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2

                readonly property int barRows: Math.max(1, height - 2)   // minus gap + label row
                readonly property int gap: width >= root.bandCount * 5 ? 2 : 1
                readonly property int barWidth: Math.max(1,
                    Math.floor((width - (root.bandCount - 1) * gap) / root.bandCount))
                readonly property int usedWidth: root.bandCount * barWidth + (root.bandCount - 1) * gap
                readonly property int leftPad: Math.max(0, Math.floor((width - usedWidth) / 2))

                // Bars grow upward from the row just above the label line.
                Column {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    width: parent.width
                    height: eqArea.barRows
                    spacing: 0

                    Repeater {
                        model: eqArea.barRows
                        delegate: Text {
                            required property int index
                            readonly property int rowFromBottom: eqArea.barRows - 1 - index
                            readonly property real heightRatio: (rowFromBottom + 1) / eqArea.barRows
                            height: 1
                            bold: true
                            text: root.spectrumRow(rowFromBottom, eqArea.barRows,
                                                   eqArea.barWidth, eqArea.gap, eqArea.leftPad)
                            color: heightRatio > 0.8 ? root.secondaryColor
                                 : heightRatio > 0.5 ? root.midColor : root.accentColor
                        }
                    }
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    height: 1
                    text: root.labelRow(eqArea.barWidth, eqArea.gap, eqArea.leftPad, eqArea.width)
                    color: root.dimColor
                }
            }
        }

        // ---- VU METERS & OSCILLOSCOPE ----
        Panel {
            id: scopePanel
            width: parent.width
            height: mainArea.height - spectrumPanel.height
            color: root.panelColor
            title: root.state === "scope" ? "LISSAJOUS PHASE SCOPE" : "BRAILLE OSCILLOSCOPE"
            titleColor: root.state === "scope" ? root.accentColor : "#94a3b8"
            border.color: root.state === "scope" ? root.accentColor : root.frameColor

            Row {
                id: scopeRow
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.bottomMargin: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 2

                // Stereo VU deck, vertically centred; drops its heading when space is tight.
                Item {
                    id: vuDeck
                    width: 28
                    height: parent.height

                    readonly property bool compact: height < 7

                    Column {
                        width: parent.width
                        height: vuDeck.compact ? 4 : 7
                        y: Math.max(0, Math.floor((vuDeck.height - height) / 2))
                        spacing: vuDeck.compact ? 0 : 1

                        Text {
                            visible: !vuDeck.compact
                            text: "STEREO VU"
                            color: root.secondaryColor
                            bold: true
                        }

                        Column {
                            spacing: 0
                            Text {
                                text: "L  " + root.toDecibels(root.vuLeft)
                                color: root.vuLeft > 0.85 ? "#ef4444" : "#94a3b8"
                            }
                            ProgressBar {
                                width: vuDeck.width
                                value: root.vuLeft * 100
                                from: 0
                                to: 100
                                thresholds: [
                                    { value: 65, color: root.accentColor },
                                    { value: 85, color: "#eab308" },
                                    { value: 100, color: "#ef4444" }
                                ]
                            }
                        }

                        Column {
                            spacing: 0
                            Text {
                                text: "R  " + root.toDecibels(root.vuRight)
                                color: root.vuRight > 0.85 ? "#ef4444" : "#94a3b8"
                            }
                            ProgressBar {
                                width: vuDeck.width
                                value: root.vuRight * 100
                                from: 0
                                to: 100
                                thresholds: [
                                    { value: 65, color: root.accentColor },
                                    { value: 85, color: "#eab308" },
                                    { value: 100, color: "#ef4444" }
                                ]
                            }
                        }
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: root.frameColor
                }

                Canvas {
                    id: scopeCanvas
                    width: Math.max(4, scopeRow.width - vuDeck.width - 1 - scopeRow.spacing * 2)
                    height: parent.height
                    autoClear: false
                    renderMode: Canvas.RenderMode.Braille
                    color: root.accentColor
                    onPixelWidthChanged: root.drawOscilloscope()
                    onPixelHeightChanged: root.drawOscilloscope()
                }
            }
        }
    }
}
