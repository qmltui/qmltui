import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    color: "black"

    // Mandatory Quit key handler
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    property real progress: 0.0

    // Animation driving progress value continuously
    NumberAnimation {
        target: root
        property: "progress"
        from: 0.0
        to: 100.0
        duration: 4000
        loops: -1
        running: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#0f172a"
        border.style: "double"
        border.color: "#38bdf8"

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                text: "📊 Qtui.Controls ProgressBar Showcase"
                color: "#38bdf8"
                bold: true
            }

            Text {
                text: "Progress: " + root.progress.toFixed(1) + "%"
                color: "#f8fafc"
                bold: true
            }

            // 1. Left-to-Right Multi-Color Gradient (Green -> Yellow -> Red across the bar length)
            Row {
                spacing: 2
                Text { width: 18; text: "LTR Gradient Bar:"; color: "#94a3b8" }
                ProgressBar {
                    width: 25
                    value: root.progress
                    from: 0
                    to: 100
                    style: "segmented"

                    gradient: Gradient {
                        GradientStop { position: 0.4; color: "#34d399" } // Green on left (0 - 40%)
                        GradientStop { position: 0.75; color: "#fbbf24" } // Yellow in middle (40 - 75%)
                        GradientStop { position: 1.0; color: "#f87171" } // Red on right (75 - 100%)
                    }
                }
            }

            // 2. Custom Arbitrary Characters (Hash / Dash: '#' & '-')
            Row {
                spacing: 2
                Text { width: 18; text: "Custom Hash/Dash:"; color: "#94a3b8" }
                ProgressBar {
                    width: 25
                    value: root.progress
                    from: 0
                    to: 100
                    fillCharacter: "#"
                    emptyCharacter: "-"
                    color: "#fbbf24"
                }
            }

            // 3. Custom Arbitrary Characters (Star / Empty Star: '★' & '☆')
            Row {
                spacing: 2
                Text { width: 18; text: "Custom Stars:"; color: "#94a3b8" }
                ProgressBar {
                    width: 25
                    value: root.progress
                    from: 0
                    to: 100
                    fillCharacter: "★"
                    emptyCharacter: "☆"

                    gradient: Gradient {
                        GradientStop { position: 0.4; color: "#38bdf8" }
                        GradientStop { position: 0.8; color: "#c084fc" }
                        GradientStop { position: 1.0; color: "#f472b6" }
                    }
                }
            }

            // 4. Dot / LED Style
            Row {
                spacing: 2
                Text { width: 18; text: "Dot LED LTR:"; color: "#94a3b8" }
                ProgressBar {
                    width: 25
                    value: root.progress
                    from: 0
                    to: 100
                    style: "dot"

                    gradient: Gradient {
                        GradientStop { position: 0.5; color: "#10b981" }
                        GradientStop { position: 0.8; color: "#f59e0b" }
                        GradientStop { position: 1.0; color: "#ef4444" }
                    }
                }
            }

            Text {
                text: "Press [Q] or [Esc] to exit"
                color: "#64748b"
            }
        }
    }
}
