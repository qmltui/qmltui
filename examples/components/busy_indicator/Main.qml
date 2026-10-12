import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Panel {
        anchors.fill: parent
        anchors.margins: 1
        title: "Activity Indicators (BusyIndicator Controls)"
        titleAlignment: Qt.AlignHCenter
        titleColor: "yellow"
        border.color: "cyan"

        Column {
            anchors.fill: parent
            anchors.topMargin: 1
            anchors.leftMargin: 2

            spacing: 1
            Row {
                spacing: 2
                BusyIndicator {
                    color: "cyan"
                }
                Text { text: "Loading network requests (Dots)"; color: "cyan" }
            }
            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["|", "/", "-", "\\"]
                    color: "green"
                }
                Text { text: "Loading system resources (Line)"; color: "green" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["┤", "┘", "┴", "└", "├", "┌", "┬", "┐"]
                    color: "yellow"
                }
                Text { text: "Encrypting sensitive data (Pipe)"; color: "yellow" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["▖", "▘", "▝", "▗"]
                    color: "magenta"
                }
                Text { text: "Initializing database connection (Block)"; color: "magenta" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["[=   ]", "[ =  ]", "[  = ]", "[   =]", "[  = ]", "[ =  ]"]
                    color: "lightgreen"
                }
                Text { text: "Connecting to remote server (Bounce)"; color: "lightgreen" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["◐", "◓", "◑", "◒"]
                    color: "lightblue"
                }
                Text { text: "Resolving domain names (Circle)"; color: "lightblue" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["←", "↖", "↑", "↗", "→", "↘", "↓", "↙"]
                    color: "lightyellow"
                }
                Text { text: "Synchronizing repository (Arrows)"; color: "lightyellow" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["░", "▒", "▓", "█", "▓", "▒"]
                    color: "lightmagenta"
                }
                Text { text: "Waiting for background worker (Pulse)"; color: "lightmagenta" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: [" ", "▃", "▄", "▅", "▆", "▇", "█", "▇", "▆", "▅", "▄", "▃"]
                    color: "cyan"
                }
                Text { text: "Processing audio stream (Grow)"; color: "cyan" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["▲", "▶", "▼", "◀"]
                    color: "green"
                }
                Text { text: "Routing network packets (Triangles)"; color: "green" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: [".  ", ".. ", "...", "   "]
                    color: "yellow"
                    interval: 500
                }
                Text { text: "Connecting to fallback host (AsciiDots)"; color: "yellow" }
            }

            Row {
                spacing: 2
                BusyIndicator {
                    frames: ["◴", "◷", "◶", "◵"]
                    color: "white"
                }
                Text { text: "Streaming AI response (Clock)"; color: "white" }
            }
        }
    }
}
