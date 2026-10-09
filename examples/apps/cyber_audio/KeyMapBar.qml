pragma ComponentBehavior: Bound
import Qtui

// One-row footer listing the active shortcuts. Hints that do not apply to the current
// source are hidden; narrow terminals get a compact variant.
Rectangle {
    id: bar
    required property AudioEngine engine
    required property string accentColor
    property bool showEqualizer: true
    property bool showScope: true
    property bool showVu: true
    property int scopeArt: 0
    property bool blendOverlaps: false
    property bool immersive: false

    // Wide labels need ~128 cells with every hint shown; compact ~109; tiny drops the
    // secondary hints so the essentials still fit an 80-column terminal.
    readonly property bool compact: width < 132
    readonly property bool tiny: width < 112
    readonly property bool fileSource: engine.source === AudioEngine.File
    readonly property bool micSource: engine.source === AudioEngine.Microphone

    height: 1
    color: Theme.barBackground

    Row {
        anchors.centerIn: parent
        spacing: bar.compact ? 1 : (bar.width >= 140 ? 3 : 2)

        KeyHint {
            key: bar.compact ? "SPC" : "SPACE"
            label: bar.engine.playing ? "Pause" : "Play"
            active: !bar.engine.playing
            activeColor: Theme.warning
        }
        KeyHint {
            visible: bar.fileSource
            key: "←→"
            label: "Seek"
            activeColor: bar.accentColor
        }
        KeyHint {
            visible: bar.engine.trackCount > 1
            key: "N/P"
            label: bar.compact ? "Trk" : "Track"
            activeColor: bar.accentColor
        }
        KeyHint {
            visible: bar.fileSource && !bar.tiny
            key: "+-"
            label: "Vol"
            activeColor: bar.accentColor
        }
        KeyHint {
            visible: !bar.immersive
            key: "1"
            label: "EQ"
            active: bar.showEqualizer
            activeColor: bar.accentColor
        }
        KeyHint {
            visible: !bar.immersive
            key: "2"
            label: "Scope"
            active: bar.showScope
            activeColor: Theme.scopeAccent
        }
        KeyHint {
            visible: !bar.immersive
            key: "3"
            label: "VU"
            active: bar.showVu
            activeColor: Theme.vuLow
        }
        KeyHint {
            visible: !bar.immersive && bar.showScope
            key: "V"
            label: ["Trace", "Glow", "Ridge", "Nebula"][bar.scopeArt]
            active: bar.scopeArt !== 0
            activeColor: Theme.scopeAccent
        }
        KeyHint {
            visible: !bar.tiny && !bar.immersive
            key: "[ ]"
            label: bar.compact ? "LP" : "Filter"
            active: bar.engine.scopeLowPassHz > 0
            activeColor: Theme.scopeAccent
        }
        KeyHint {
            visible: !bar.tiny && !bar.immersive
            key: "A"
            label: "Blend"
            active: bar.blendOverlaps
            activeColor: Theme.scopeRight
        }
        KeyHint {
            visible: bar.engine.multimediaAvailable
            key: "M"
            label: "Mic"
            active: bar.micSource
            activeColor: Theme.micSource
        }
        KeyHint {
            key: "F"
            label: bar.immersive ? "Back" : "Art"
            active: bar.immersive
            activeColor: bar.accentColor
        }
        KeyHint {
            key: "Q"
            label: "Quit"
            activeColor: bar.accentColor
        }
    }
}
