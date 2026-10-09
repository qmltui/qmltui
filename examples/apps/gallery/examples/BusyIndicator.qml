pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// BusyIndicator: Five spinner styles. One tile of the Qtui gallery (examples/apps/gallery)
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
        Text { text: "Each style advances every `interval` milliseconds."; color: "#6e7b8b" }
        Row {
            spacing: 4
            Row { spacing: 1; BusyIndicator { style: BusyIndicator.Style.Dots; color: "#f472b6" } Text { text: "Dots"; color: "#c9d1d9" } }
            Row { spacing: 1; BusyIndicator { style: BusyIndicator.Style.Line; color: "#f472b6" } Text { text: "Line"; color: "#c9d1d9" } }
            Row { spacing: 1; BusyIndicator { style: BusyIndicator.Style.Pipe; color: "#f472b6" } Text { text: "Pipe"; color: "#c9d1d9" } }
            Row { spacing: 1; BusyIndicator { style: BusyIndicator.Style.Block; color: "#f472b6" } Text { text: "Block"; color: "#c9d1d9" } }
            Row { spacing: 1; BusyIndicator { style: BusyIndicator.Style.Bounce; color: "#f472b6" } Text { text: "Bounce"; color: "#c9d1d9" } }
        }
        Row {
            spacing: 1
            BusyIndicator { style: BusyIndicator.Style.Dots; interval: 40; color: "#7ee0ff" }
            Text { text: "interval: 40 ms"; color: "#6e7b8b" }
        }
        Row {
            spacing: 1
            BusyIndicator { id: paused; style: BusyIndicator.Style.Line; running: false; color: "#6e7b8b" }
            Text { text: "running: false keeps the current glyph"; color: "#6e7b8b" }
        }
    }
}
