pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// ProgressBar: Visualizes completion ratios with block, segmented, dot, or custom glyphs.
Column {
    id: progressDemo

    property real progress: 0

    anchors.centerIn: parent
    spacing: 1

    // Drive all bars from a looping 0..100 animation
    NumberAnimation {
        target: progressDemo
        property: "progress"
        from: 0
        to: 100
        duration: 5000
        loops: -1
        running: true
    }

    Text {
        text: "Value " + progressDemo.progress.toFixed(0) + "%, driven by a looping NumberAnimation."
        color: "#8b949e"
    }

    Row {
        spacing: 2

        Text {
            width: 10
            text: "block"
            color: "#c9d1d9"
        }

        ProgressBar {
            width: 30
            from: 0
            to: 100
            value: progressDemo.progress
            color: "#58a6ff"
            showBrackets: true
            showPercentage: true
        }
    }

    Row {
        spacing: 2

        Text {
            width: 10
            text: "segmented"
            color: "#c9d1d9"
        }

        ProgressBar {
            width: 30
            from: 0
            to: 100
            value: progressDemo.progress
            style: "segmented"
            gradient: Gradient {
                GradientStop {
                    position: 0.4
                    color: "#2cde85"
                }
                GradientStop {
                    position: 0.75
                    color: "#fbbf24"
                }
                GradientStop {
                    position: 1.0
                    color: "#f87171"
                }
            }
        }
    }

    Row {
        spacing: 2

        Text {
            width: 10
            text: "thresholds"
            color: "#c9d1d9"
        }

        ProgressBar {
            width: 30
            from: 0
            to: 100
            value: progressDemo.progress
            style: "dot"
            thresholds: [
                {
                    value: 50,
                    color: "#2cde85"
                },
                {
                    value: 80,
                    color: "#fbbf24"
                },
                {
                    value: 100,
                    color: "#f87171"
                }
            ]
        }
    }

    Row {
        spacing: 2

        Text {
            width: 10
            text: "custom"
            color: "#c9d1d9"
        }

        ProgressBar {
            width: 30
            from: 0
            to: 100
            value: progressDemo.progress
            fillCharacter: "★"
            emptyCharacter: "☆"
            color: "#d2a8ff"
        }
    }
}
