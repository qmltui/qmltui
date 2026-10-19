pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// BusyIndicator: Multi-frame terminal spinner advancing every `interval` ms.
Column {
    spacing: 1

    Text {
        text: "Frames advance every `interval` milliseconds while running is true."
        color: "#8b949e"
    }

    // Built-in Braille spinner and custom frame sequences
    Row {
        spacing: 3

        Row {
            spacing: 1

            BusyIndicator {
                color: "#f472b6"
            }

            Text {
                text: "Dots"
                color: "#c9d1d9"
            }
        }

        Row {
            spacing: 1

            BusyIndicator {
                frames: ["|", "/", "-", "\\"]
                color: "#f472b6"
            }

            Text {
                text: "Line"
                color: "#c9d1d9"
            }
        }

        Row {
            spacing: 1

            BusyIndicator {
                frames: ["▖", "▘", "▝", "▗"]
                color: "#f472b6"
            }

            Text {
                text: "Block"
                color: "#c9d1d9"
            }
        }

        Row {
            spacing: 1

            BusyIndicator {
                frames: ["[=   ]", "[ =  ]", "[  = ]", "[   =]", "[  = ]", "[ =  ]"]
                color: "#f472b6"
            }

            Text {
                text: "Bounce"
                color: "#c9d1d9"
            }
        }
    }

    Row {
        spacing: 1

        BusyIndicator {
            interval: 40
            color: "#7ee0ff"
        }

        Text {
            text: "interval: 40 ms (fast)"
            color: "#8b949e"
        }
    }

    Row {
        spacing: 1

        BusyIndicator {
            frames: ["|", "/", "-", "\\"]
            running: false
            color: "#8b949e"
        }

        Text {
            text: "running: false pauses on the current glyph"
            color: "#8b949e"
        }
    }
}
