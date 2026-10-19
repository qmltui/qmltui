pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Panel: Bordered container with Item slots for title, titleTrailing,
// footer, and footerTrailing along its top and bottom border edges.
Column {
    spacing: 1

    Row {
        spacing: 1

        // Single-line border with default left-aligned title slot
        Panel {
            width: 21
            height: 5
            border {
                style: Border.Single
                color: "#94a3b8"
            }
            title: Text {
                text: "single"
                color: "#94a3b8"
            }

            Text {
                x: 2
                y: 1
                text: "Default caption"
                color: "#c9d1d9"
            }

            Text {
                x: 2
                y: 2
                text: "left aligned"
                color: "#8b949e"
            }
        }

        // Double-line border with centered title slot
        Panel {
            width: 21
            height: 5
            titleAlignment: Qt.AlignHCenter
            border {
                style: Border.Double
                color: "#7ee0ff"
            }
            title: Text {
                text: "double"
                color: "#7ee0ff"
                bold: true
            }

            Text {
                x: 2
                y: 1
                text: "Centred caption"
                color: "#c9d1d9"
            }

            Text {
                x: 2
                y: 2
                text: "coloured border"
                color: "#8b949e"
            }
        }
    }

    // Rounded border with right-aligned title and footer slot
    Panel {
        width: 43
        height: 5
        color: "#1a1a10"
        titleAlignment: Qt.AlignRight
        border {
            style: Border.Rounded
            color: "#f9c74f"
        }
        title: Text {
            text: "rounded"
            color: "#f9c74f"
        }
        footer: Text {
            text: "v1.0"
            color: "#8b949e"
        }

        Text {
            x: 2
            y: 1
            text: "Right caption"
            color: "#c9d1d9"
        }

        Text {
            x: 2
            y: 2
            text: "filled background"
            color: "#8b949e"
        }
    }
}
