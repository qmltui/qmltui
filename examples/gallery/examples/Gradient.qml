pragma ComponentBehavior: Bound
import QmlTui

// Gradient: Multi-stop horizontal and vertical linear color gradients.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Linear color interpolation across character cells using GradientStop."
        color: "#8b949e"
    }

    Row {
        spacing: 2

        Rectangle {
            width: 26
            height: 5
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "#0ea5e9"
                }
                GradientStop {
                    position: 1.0
                    color: "#6366f1"
                }
            }

            Text {
                anchors.centerIn: parent
                text: "Gradient.Horizontal"
                color: "white"
                bold: true
            }
        }

        Rectangle {
            width: 26
            height: 5
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop {
                    position: 0.0
                    color: "#10b981"
                }
                GradientStop {
                    position: 1.0
                    color: "#0f766e"
                }
            }

            Text {
                anchors.centerIn: parent
                text: "Gradient.Vertical"
                color: "white"
                bold: true
            }
        }
    }

    // Four-stop horizontal spectrum bar
    Rectangle {
        width: 54
        height: 3
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: "#ef4444"
            }
            GradientStop {
                position: 0.33
                color: "#eab308"
            }
            GradientStop {
                position: 0.66
                color: "#22c55e"
            }
            GradientStop {
                position: 1.0
                color: "#3b82f6"
            }
        }

        Text {
            anchors.centerIn: parent
            text: "Four-Stop Spectrum (0.0 → 0.33 → 0.66 → 1.0)"
            color: "white"
            bold: true
        }
    }
}
