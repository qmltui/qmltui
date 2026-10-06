pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Extras

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Rectangle {
        id: root
        width: 78
        height: 24
        color: "#0a0e14"

        // Mandatory Qtui quit key handling

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
                        text: "◈ GAUGE COMPONENT & ANIMATION SHOWCASE"
                        color: "#38bdf8"
                        bold: true
                    }
                    Text {
                        text: "[Press 'Q' / 'Esc' to quit]"
                        color: "#64748b"
                    }
                }

                Text {
                    text: "Demonstrates animated values, moving TrueColor gradients,\ncustom glyphs & borders."
                    color: "#94a3b8"
                }

                // Section 1: Animated Live Telemetry
                Text {
                    text: "─── 1. Live Animated Telemetry ──────────────────────────────"
                    color: "#64748b"
                }

                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: "single"
                    border.color: "#475569"

                    Gauge {
                        anchors.fill: parent
                        anchors.margins: 1
                        text: "CPU LOAD   : "
                        value: root.cpuValue
                        to: 100.0
                        color: root.cpuValue > 80 ? "#ef4444" : (root.cpuValue > 60 ? "#f59e0b" : "#10b981")
                    }
                }

                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: "single"
                    border.color: "#334155"

                    Gauge {
                        anchors.fill: parent
                        anchors.margins: 1
                        text: "SWEEP SCAN : "
                        value: root.sweepProgress
                        to: 100.0
                        color: "#06b6d4"
                        fillCharacter: "▰"
                        emptyCharacter: "▱"
                    }
                }

                // Section 2: Moving Color Gradients (via Qtui Gradient & Offset Animation)
                Text {
                    text: "─── 2. Moving Color Gradients (Animated Wave) ───────────────"
                    color: "#64748b"
                }

                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: "single"
                    border.color: "#818cf8"

                    Gauge {
                        anchors.fill: parent
                        anchors.margins: 1
                        text: "CYBER WAVE : "
                        value: 85.0
                        to: 100.0
                        fillCharacter: "█"
                        emptyCharacter: "░"
                        showBrackets: false
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
                }

                Rectangle {
                    width: parent.width - 2
                    height: 3
                    border.style: "single"
                    border.color: "#64748b"

                    Gauge {
                        anchors.fill: parent
                        anchors.margins: 1
                        text: "TEMPERATURE: "
                        value: root.cpuValue
                        to: 100.0
                        showBrackets: true
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
                }

                // Section 3: Custom Glyphs & Minimal Style
                Text {
                    text: "─── 3. Custom Glyphs & Minimal Style ────────────────────────"
                    color: "#64748b"
                }

                Gauge {
                    width: parent.width - 2
                    text: "MEMORY VIRT: "
                    value: 64.0
                    to: 100.0
                    color: "#a855f7"
                    fillCharacter: "■"
                    emptyCharacter: "·"
                    showBrackets: false
                }
            }
        }
    }
}
