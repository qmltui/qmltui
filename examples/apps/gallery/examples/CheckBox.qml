pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// CheckBox: Toggle with Space. One tile of the Qtui gallery (examples/apps/gallery)
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
        Text { text: "Space or Enter toggles the focused box."; color: "#6e7b8b" }
        CheckBox { id: logging; text: "Enable logging"; checked: true; focus: true }
        CheckBox { id: verbose; text: "Verbose output" }
        CheckBox { id: colors; text: "Colour output"; checked: true; textColor: "#7ee0ff" }
        Text {
            text: "logging=" + logging.checked + "  verbose=" + verbose.checked + "  colors=" + colors.checked
            color: "#7ee0ff"
        }
    }
}
