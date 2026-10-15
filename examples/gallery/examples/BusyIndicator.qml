pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// BusyIndicator: Five spinner styles. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text {
            text: "Frames advance every `interval` milliseconds."
            color: "#8b949e"
        }
        Row {
            spacing: 4
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
                    frames: ["┤", "┘", "┴", "└", "├", "┌", "┬", "┐"]
                    color: "#f472b6"
                }
                Text {
                    text: "Pipe"
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
                text: "interval: 40 ms"
                color: "#8b949e"
            }
        }
        Row {
            spacing: 1
            BusyIndicator {
                id: paused
                frames: ["|", "/", "-", "\\"]
                running: false
                color: "#8b949e"
            }
            Text {
                text: "running: false keeps the current glyph"
                color: "#8b949e"
            }
        }
    }
}
