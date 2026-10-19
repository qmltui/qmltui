pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// CheckBox: Independent boolean toggles with live property bindings.
// Space, Enter, or a mouse click toggles the checked state.
Column {
    spacing: 1

    Text {
        text: "Space or Enter toggles the focused CheckBox."
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

    // Explicit textColor overrides stay active across focus changes
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
