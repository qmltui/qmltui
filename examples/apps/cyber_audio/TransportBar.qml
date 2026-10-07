pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// One-row transport strip: seek bar for files, status text otherwise, errors in red.
Rectangle {
    id: bar
    required property AudioEngine engine
    required property string sourceColor

    readonly property bool fileSource: engine.source === AudioEngine.File
    readonly property bool showError: engine.errorText.length > 0
    readonly property bool showSeek: !showError && fileSource
    readonly property string durationText: engine.formatTime(engine.duration)

    height: 1
    color: Theme.transportBackground

    Text {
        visible: bar.showError
        x: 1
        text: Theme.fitText("✖ " + bar.engine.errorText, Math.max(0, bar.width - 2))
        color: Theme.error
        bold: true
    }

    Text {
        visible: !bar.showError && !bar.showSeek
        x: 1
        text: Theme.fitText(bar.engine.statusText, Math.max(0, bar.width - 2))
        color: Theme.muted
    }

    Row {
        visible: bar.showSeek
        anchors.left: parent.left
        anchors.leftMargin: 1
        spacing: 1

        Text {
            text: bar.engine.formatTime(bar.engine.position)
            color: Theme.subtle
        }
        ProgressBar {
            width: Math.max(4, bar.width - 2 - 2 * (bar.durationText.length + 1))
            value: bar.engine.position
            from: 0
            to: Math.max(1, bar.engine.duration)
            style: "block"
            color: bar.sourceColor
        }
        Text {
            text: bar.durationText
            color: Theme.subtle
        }
    }
}
