pragma ComponentBehavior: Bound
import Qamui

// One-row header: app name, source badge, track, title, layout / play state, volume.
Rectangle {
    id: bar
    required property AudioEngine engine
    required property string accentColor
    required property string secondaryColor
    required property string sourceColor

    readonly property bool fileSource: engine.source === AudioEngine.File
    readonly property bool demoSource: engine.source === AudioEngine.Demo
    readonly property string nowPlaying: engine.artist.length > 0
                                         ? engine.title + " — " + engine.artist
                                         : engine.title

    height: 1
    color: Theme.barBackground

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 1
        spacing: 2

        Text {
            text: "⚡ NEURAL AUDIO MATRIX"
            color: bar.accentColor
            bold: true
        }
        Text {
            text: " " + bar.engine.sourceName + " "
            reversed: true
            bold: true
            color: bar.sourceColor
        }
        Text {
            visible: bar.fileSource && bar.engine.trackCount > 0
            text: "│ TRACK " + (bar.engine.trackIndex + 1) + "/" + bar.engine.trackCount
            color: Theme.muted
        }
        Text {
            // Cells left after the fixed header segments and the right-hand status.
            visible: bar.fileSource && text.length > 2
            text: "│ " + Theme.fitText(bar.nowPlaying, Math.max(0, bar.width - 84))
            color: Theme.text
            bold: true
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 1
        spacing: 2

        Text {
            visible: bar.demoSource
            text: bar.engine.beatPulse > 0.5 ? "● BEAT" : "○ beat"
            color: bar.engine.beatPulse > 0.5 ? bar.secondaryColor : Theme.dim
            bold: bar.engine.beatPulse > 0.5
        }
        Text {
            text: bar.engine.playing ? "▶ PLAYING" : "⏸ PAUSED"
            color: bar.engine.playing ? Theme.playing : Theme.warning
            bold: true
        }
        Text {
            visible: bar.fileSource
            text: "VOL " + Math.round(bar.engine.volume * 100) + "%"
            color: Theme.muted
        }
    }
}
