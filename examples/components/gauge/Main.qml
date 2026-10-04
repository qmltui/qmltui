import QtQml 2.15
import Qtui
import Qtui.Extras

Terminal {
    id: root
    fillTerminal: true
    width: 78
    height: 24
    color: "#0a0e14"

    // Mandatory Qtui quit key handling
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    // Animation driving moving gradient offset
    property real gradientOffset: 0.0
    NumberAnimation {
        target: root
        property: "gradientOffset"
        from: 0.0
        to: 1.0
        duration: 2500
        loops: -1
        running: true
    }

    // Animation simulating dynamic live CPU / GPU telemetry
    property real cpuValue: 20.0
    NumberAnimation {
        id: cpuAnimUp
        target: root
        property: "cpuValue"
        from: 20.0
        to: 94.0
        duration: 3000
        easing: Easing.InOutQuad
        running: true
        onFinished: {
            cpuAnimDown.start();
        }
    }

    NumberAnimation {
        id: cpuAnimDown
        target: root
        property: "cpuValue"
        from: 94.0
        to: 20.0
        duration: 2500
        easing: Easing.InOutQuad
        running: false
        onFinished: {
            cpuAnimUp.start();
        }
    }

    // Animation simulating continuous sweep progress
    property real sweepProgress: 0.0
    NumberAnimation {
        target: root
        property: "sweepProgress"
        from: 0.0
        to: 100.0
        duration: 4000
        loops: -1
        running: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#0f141c"
        border.style: "double"
        border.color: "#38bdf8"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // Header Section
            Row {
                spacing: 2
                Text {
                    text: "🎛️  GAUGE COMPONENT & ANIMATION SHOWCASE"
                    color: "#38bdf8"
                    bold: true
                }
                Text {
                    text: "[Press 'Q' / 'Esc' to quit]"
                    color: "#64748b"
                }
            }

            Text {
                text: "Demonstrates animated values, moving TrueColor gradients,\n custom glyphs & borders."
                color: "#94a3b8"
            }

            // Section 1: Animated Live Telemetry
            Text {
                text: "─── 1. Live Animated Telemetry ──────────────────────────────"
                color: "#64748b"
            }

            Gauge {
                width: parent.width - 2
                label: "CPU LOAD   : "
                value: root.cpuValue
                maxValue: 100.0
                gaugeColor: root.cpuValue > 80 ? "#ef4444" : (root.cpuValue > 60 ? "#f59e0b" : "#10b981")
                borderStyle: "rounded"
                borderColor: "#475569"
            }

            Gauge {
                width: parent.width - 2
                label: "SWEEP SCAN : "
                value: root.sweepProgress
                maxValue: 100.0
                gaugeColor: "#06b6d4"
                filledChar: "▰"
                unfilledChar: "▱"
                borderStyle: "single"
                borderColor: "#334155"
            }

            // Section 2: Moving Color Gradients (via Qtui Gradient & Offset Animation)
            Text {
                text: "─── 2. Moving Color Gradients (Animated Wave) ───────────────"
                color: "#64748b"
            }

            Gauge {
                width: parent.width - 2
                label: "CYBER WAVE : "
                value: 85.0
                maxValue: 100.0
                filledChar: "█"
                unfilledChar: "░"
                showBrackets: false
                borderStyle: "single"
                borderColor: "#818cf8"
                gradient: Gradient {
                    offset: root.gradientOffset
                    GradientStop {
                        position: 0.00
                        color: "#06b6d4"
                    }
                    GradientStop {
                        position: 0.33
                        color: "#ec4899"
                    }
                    GradientStop {
                        position: 0.66
                        color: "#eab308"
                    }
                    GradientStop {
                        position: 1.00
                        color: "#06b6d4"
                    }
                }
            }

            Gauge {
                width: parent.width - 2
                label: "TEMPERATURE: "
                value: root.cpuValue
                maxValue: 100.0
                showBrackets: true
                borderStyle: "rounded"
                borderColor: "#64748b"
                gradient: Gradient {
                    GradientStop {
                        position: 0.00
                        color: "#34d399"
                    }
                    GradientStop {
                        position: 0.55
                        color: "#facc15"
                    }
                    GradientStop {
                        position: 0.85
                        color: "#f87171"
                    }
                }
            }

            // Section 3: Custom Glyphs & Minimal Style
            Text {
                text: "─── 3. Custom Glyphs & Minimal Style ────────────────────────"
                color: "#64748b"
            }

            Gauge {
                width: parent.width - 2
                label: "MEMORY VIRT: "
                value: 64.0
                maxValue: 100.0
                gaugeColor: "#a855f7"
                filledChar: "■"
                unfilledChar: "·"
                showBrackets: false
                borderStyle: "none"
            }
        }
    }
}
