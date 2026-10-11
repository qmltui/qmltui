pragma ComponentBehavior: Bound
import Qamui

// Text: Styles, spans, gradients. One tile of the Qamui gallery (examples/gallery)
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
        x: 2
        y: 1
        spacing: 1
        Row {
            spacing: 2
            Text { text: "plain"; color: "#c9d1d9" }
            Text { text: "bold"; bold: true; color: "#c9d1d9" }
            Text { text: "italic"; italic: true; color: "#c9d1d9" }
            Text { text: "underline"; underline: true; color: "#c9d1d9" }
            Text { text: "dim"; dim: true; color: "#c9d1d9" }
            Text { text: "reversed"; reversed: true; color: "#c9d1d9" }
        }
        Text {
            spans: [
                { text: "spans: ", dim: true },
                { text: " READY ", color: "#001b10", backgroundColor: "#65e6a0", bold: true },
                { text: "  one Text, several styled runs", color: "#7ee0ff" },
                { text: "  ★", color: "#f9c74f" }
            ]
        }
        Text {
            text: "<b>rich</b> text with <font color='#f87171'>colour</font> and <i>emphasis</i>"
            textFormat: Text.RichText
        }
        Text {
            text: "gradient text flows from one colour to the next across the string"
            bold: true
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#7ee0ff" }
                GradientStop { position: 0.5; color: "#c084fc" }
                GradientStop { position: 1.0; color: "#f472b6" }
            }
        }
        Text {
            width: 40
            text: "right aligned in 40 cells"
            horizontalAlignment: Qt.AlignRight
            color: "#6e7b8b"
        }
    }
}
