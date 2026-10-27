pragma ComponentBehavior: Bound
import QmlTui

// Text: Terminal cell typography, spans, rich text, gradients, and alignment.
Column {
    anchors.centerIn: parent
    spacing: 1

    // Character style attributes
    Row {
        spacing: 2

        Text {
            text: "plain"
            color: "#c9d1d9"
        }

        Text {
            text: "bold"
            bold: true
            color: "#c9d1d9"
        }

        Text {
            text: "italic"
            italic: true
            color: "#c9d1d9"
        }

        Text {
            text: "underline"
            underline: true
            color: "#c9d1d9"
        }

        Text {
            text: "dim"
            dim: true
            color: "#c9d1d9"
        }

        Text {
            text: "reversed"
            reversed: true
            color: "#c9d1d9"
        }
    }

    // Multi-span text in a single item
    Text {
        spans: [
            {
                text: "spans: ",
                dim: true
            },
            {
                text: " READY ",
                color: "#001b10",
                backgroundColor: "#2cde85",
                bold: true
            },
            {
                text: "  styled runs",
                color: "#7ee0ff"
            },
            {
                text: "  ★",
                color: "#f9c74f"
            }
        ]
    }

    // Inline HTML subset formatting
    Text {
        text: "<b>rich</b> text with <font color='#f87171'>colour</font> and <i>emphasis</i>"
        textFormat: Text.RichText
    }

    // Per-character horizontal color gradient
    Text {
        text: "gradient text flows across the string"
        bold: true
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: "#7ee0ff"
            }
            GradientStop {
                position: 0.5
                color: "#c084fc"
            }
            GradientStop {
                position: 1.0
                color: "#f472b6"
            }
        }
    }

    // Animated moving gradient (Gradient.offset loops 0.0 -> 1.0)
    Text {
        text: "✦ moving gradient wave across terminal glyphs ✦"
        bold: true
        gradient: Gradient {
            id: movingGrad
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0.0
                color: "#2cde85"
            }
            GradientStop {
                position: 0.33
                color: "#38bdf8"
            }
            GradientStop {
                position: 0.66
                color: "#f472b6"
            }
            GradientStop {
                position: 1.0
                color: "#2cde85"
            }
        }

        NumberAnimation {
            target: movingGrad
            property: "offset"
            from: 0.0
            to: 1.0
            duration: 2400
            loops: -1
            running: true
        }
    }

    Text {
        width: 40
        text: "right aligned in 40 cells"
        horizontalAlignment: Qt.AlignRight
        color: "#8b949e"
    }
}
