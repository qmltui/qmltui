pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// RadioButton: One choice per group. One tile of the Qamui gallery (examples/apps/gallery)
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
        Text { text: "One choice at a time; the ButtonGroup keeps them exclusive."; color: "#6e7b8b" }
        ButtonGroup { buttons: [fast, balanced, safe] }
        Row {
            spacing: 3
            RadioButton { id: fast; text: "Fast"; checked: true; focus: true }
            RadioButton { id: balanced; text: "Balanced" }
            RadioButton { id: safe; text: "Safe" }
        }
        Text {
            text: "Profile: " + (fast.checked ? "fast (no fsync)" : balanced.checked ? "balanced" : "safe (fsync every write)")
            color: "#f9c74f"
        }
    }
}
