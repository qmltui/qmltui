pragma ComponentBehavior: Bound
import QtQml
import Qtui

// Composition root: owns the AudioEngine, the keyboard map and the widget toggles, and
// arranges the HeaderBar / TransportBar / SpectrumPanel / ScopePanel / KeyMapBar.
Terminal {
    id: root
    title: "// CYBERPUNK 2077 // NEURAL AUDIO MATRIX"
    color: Theme.background
    focus: true

    // ==========================================
    // AUDIO ENGINE (C++): demo synth, media files, microphone
    // ==========================================
    AudioEngine {
        id: engine
    }

    Component.onCompleted: {
        // cliOptions is a context property set by main.cpp / main_test.cpp.
        if (cliOptions.volume !== undefined)
            engine.volume = cliOptions.volume
        if (cliOptions.inputs && cliOptions.inputs.length > 0 && !cliOptions.demo)
            engine.loadPlaylist(cliOptions.inputs)
        else if (cliOptions.mic)
            engine.useMicrophone()
        else if (cliOptions.demo)
            engine.useDemo()
        // Otherwise the engine stays idle and the transport row says how to start.
    }

    // ==========================================
    // KEYBOARD INPUT & QUIT PATH (AGENTS.md)
    // ==========================================
    // Named keys (Escape, Space, arrows) arrive with a key code; printable keys are
    // matched on event.text so scripted input works the same as a real terminal.
    Keys.onPressed: (event) => {
        let handled = true
        switch (event.key) {
        case Qt.Key_Escape:
            if (root.immersive)
                root.immersive = false
            else
                Qt.quit()
            break
        case Qt.Key_Space:
            engine.togglePlayback()
            break
        case Qt.Key_Left:
            engine.seek(-5000)
            break
        case Qt.Key_Right:
            engine.seek(5000)
            break
        default:
            switch (event.text ? event.text.toLowerCase() : "") {
            case "q": Qt.quit(); break
            case " ": engine.togglePlayback(); break
            case "1": root.showEqualizer = !root.showEqualizer; break
            case "2": root.showScope = !root.showScope; break
            case "3": root.showVu = !root.showVu; break
            case "a": root.blendOverlaps = !root.blendOverlaps; break
            case "v": root.scopeArt = (root.scopeArt + 1) % 4; root.immersive = false; break
            case "f": root.immersive = !root.immersive; break
            case "[": engine.stepScopeLowPass(-1); break
            case "]": engine.stepScopeLowPass(1); break
            case "n": engine.next(); break
            case "p": engine.previous(); break
            case "+":
            case "=": engine.adjustVolume(0.05); break
            case "-": engine.adjustVolume(-0.05); break
            case "m": engine.useMicrophone(); break
            default: handled = false
            }
        }
        event.accepted = handled
    }

    // ==========================================
    // WIDGET TOGGLES & ACCENTS
    // ==========================================
    property bool immersive: false
    property bool showEqualizer: true
    property bool showScope: true
    property bool showVu: true
    readonly property bool showLowerPanel: showScope || showVu

    // Oscilloscope cells shared by both traces: average the colors (A) or let left win.
    property bool blendOverlaps: false
    // Oscilloscope rendering: 0 traces, 1 persistence, 2 ridge, 3 nebula.
    property int scopeArt: 0

    // Share of the main area given to the equalizer panel (0..1). Driven by the layout
    // states below so toggling a panel animates the other one into the freed space.
    property real spectrumShare: 0.55

    // Plain bindings on purpose: a `Behavior on` a bound string property does not apply
    // the new value in Qtui (verified 2026-09).
    readonly property string accentColor: Theme.accent
    readonly property string secondaryColor: Theme.secondary
    readonly property string scopeAccent: Theme.scopeAccent
    readonly property string sourceColor: engine.source === AudioEngine.File ? Theme.fileSource
                                        : engine.source === AudioEngine.Microphone ? Theme.micSource
                                        : engine.source === AudioEngine.Idle ? Theme.muted
                                        : root.accentColor

    state: root.immersive ? "lower" : root.showEqualizer && root.showLowerPanel ? "both"
         : root.showEqualizer ? "equalizer"
         : root.showLowerPanel ? "lower" : "none"

    states: [
        State {
            name: "both"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.55 } }
        },
        State {
            name: "equalizer"
            PropertyChanges { target: root; properties: { "spectrumShare": 1.0 } }
        },
        State {
            name: "lower"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.0 } }
        },
        State {
            name: "none"
            PropertyChanges { target: root; properties: { "spectrumShare": 0.0 } }
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
    // SCREEN LAYOUT
    // ==========================================
    HeaderBar {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        engine: engine
        accentColor: root.accentColor
        secondaryColor: root.secondaryColor
        sourceColor: root.sourceColor
    }

    TransportBar {
        id: transport
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        engine: engine
        sourceColor: root.sourceColor
    }

    KeyMapBar {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        engine: engine
        accentColor: root.accentColor
        showEqualizer: root.showEqualizer
        showScope: root.showScope
        showVu: root.showVu
        blendOverlaps: root.blendOverlaps
        scopeArt: root.immersive ? 3 : root.scopeArt
        immersive: root.immersive
    }

    // The visible panels share the remaining height according to spectrumShare.
    Column {
        id: mainArea
        anchors.top: transport.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        readonly property int minSpectrumHeight: 6
        readonly property int minLowerHeight: 7
        readonly property int spectrumHeight: {
            const share = Math.max(0, Math.min(1, root.spectrumShare))
            let h = Math.round(height * share)
            if (!root.immersive && root.showEqualizer && root.showLowerPanel)
                h = Math.max(minSpectrumHeight, Math.min(height - minLowerHeight, h))
            return Math.max(0, Math.min(height, h))
        }

        SpectrumPanel {
            visible: !root.immersive && root.showEqualizer
            width: parent.width
            height: mainArea.spectrumHeight
            engine: engine
            accentColor: root.accentColor
            secondaryColor: root.secondaryColor
        }

        ScopePanel {
            visible: root.immersive || root.showLowerPanel
            width: parent.width
            height: mainArea.height - (!root.immersive && root.showEqualizer ? mainArea.spectrumHeight : 0)
            engine: engine
            accentColor: root.scopeAccent
            secondaryColor: root.secondaryColor
            blendOverlaps: root.blendOverlaps
            scopeArt: root.immersive ? 3 : root.scopeArt
            showScope: root.immersive || root.showScope
            showVu: !root.immersive && root.showVu
        }

        Text {
            visible: !root.immersive && !root.showEqualizer && !root.showLowerPanel
            x: Math.max(0, Math.floor((mainArea.width - text.length) / 2))
            y: Math.floor(mainArea.height / 2)
            text: "All panels hidden · 1 equalizer · 2 oscilloscope · 3 VU meters"
            color: Theme.muted
        }
    }
}
