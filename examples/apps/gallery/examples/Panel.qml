pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// Panel: Bordered box with title. One tile of the Qtui gallery (examples/apps/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Row {
        x: 2
        y: 1
        spacing: 2
        Panel {
            width: 24; height: 6
            title: "single"
            border.style: "single"
            border.color: "#94a3b8"
            Text { x: 2; y: 1; text: "Default caption"; color: "#c9d1d9" }
            Text { x: 2; y: 2; text: "left aligned"; color: "#6e7b8b" }
        }
        Panel {
            width: 24; height: 6
            title: "double"
            titleAlignment: Qt.AlignHCenter
            titleColor: "#7ee0ff"
            border.style: "double"
            border.color: "#7ee0ff"
            Text { x: 2; y: 1; text: "Centred caption"; color: "#c9d1d9" }
            Text { x: 2; y: 2; text: "coloured border"; color: "#6e7b8b" }
        }
        Panel {
            width: 24; height: 6
            title: "bold"
            titleAlignment: Qt.AlignRight
            titleColor: "#f9c74f"
            border.style: "bold"
            border.color: "#f9c74f"
            color: "#1a1a10"
            Text { x: 2; y: 1; text: "Right caption"; color: "#c9d1d9" }
            Text { x: 2; y: 2; text: "filled background"; color: "#6e7b8b" }
        }
    }
}
