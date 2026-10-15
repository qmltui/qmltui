pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// CheckBox: Toggle with Space. One tile of the QmlTui gallery (examples/gallery)
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
            text: "Space or Enter toggles the focused box."
            color: "#8b949e"
        }
        CheckBox {
            id: logging
            text: "Enable logging"
            checked: true
            focus: true
        }
        CheckBox {
            id: verbose
            text: "Verbose output"
        }
        CheckBox {
            id: colors
            text: "Colour output"
            checked: true
            textColor: "#7ee0ff"
        }
        Text {
            text: "logging=" + logging.checked + "  verbose=" + verbose.checked + "  colors=" + colors.checked
            color: "#7ee0ff"
        }
    }
}
