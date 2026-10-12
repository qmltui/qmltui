pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// ProgressBar: Styles and gradients. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        id: progressDemo
        x: 2
        y: 1
        property real progress: 0
        spacing: 1
        NumberAnimation {
            target: progressDemo
            property: "progress"
            from: 0; to: 100
            duration: 5000
            loops: -1
            running: true
        }
        Text { text: "Value " + progressDemo.progress.toFixed(0) + "%, driven by a looping NumberAnimation."; color: "#6e7b8b" }
        Row {
            spacing: 2
            Text { width: 12; text: "block"; color: "#c9d1d9" }
            ProgressBar { width: 40; from: 0; to: 100; value: progressDemo.progress; color: "#38bdf8"; showBrackets: true; showPercentage: true }
        }
        Row {
            spacing: 2
            Text { width: 12; text: "segmented"; color: "#c9d1d9" }
            ProgressBar {
                width: 40; from: 0; to: 100; value: progressDemo.progress; style: "segmented"
                gradient: Gradient {
                    GradientStop { position: 0.4; color: "#34d399" }
                    GradientStop { position: 0.75; color: "#fbbf24" }
                    GradientStop { position: 1.0; color: "#f87171" }
                }
            }
        }
        Row {
            spacing: 2
            Text { width: 12; text: "thresholds"; color: "#c9d1d9" }
            ProgressBar {
                width: 40; from: 0; to: 100; value: progressDemo.progress; style: "dot"
                thresholds: [{ value: 50, color: "#34d399" }, { value: 80, color: "#fbbf24" }, { value: 100, color: "#f87171" }]
            }
        }
        Row {
            spacing: 2
            Text { width: 12; text: "custom"; color: "#c9d1d9" }
            ProgressBar { width: 40; from: 0; to: 100; value: progressDemo.progress; fillCharacter: "★"; emptyCharacter: "☆"; color: "#c084fc" }
        }
    }
}
