pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// CheckBox: Independent boolean toggles with default [x]/[ ] and custom [ ON]/[OFF] indicators.
// Space, Enter, or a mouse click toggles the checked state.
Column {
    anchors.centerIn: parent
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

    // Custom contentItem rendering an [ ON] / [OFF] switch indicator
    CheckBox {
        id: autoSync
        text: "Auto-sync (custom [ ON]/[OFF] contentItem)"
        checked: true
        contentItem: Row {
            spacing: 1

            Text {
                text: autoSync.checked ? "[ ON]" : "[OFF]"
                color: autoSync.checked ? "#2cde85" : "#8b949e"
                bold: autoSync.activeFocus || autoSync.hovered || autoSync.highlighted
                reversed: autoSync.activeFocus
            }

            Text {
                text: autoSync.text
                color: "#c9d1d9"
                bold: autoSync.activeFocus || autoSync.hovered || autoSync.highlighted
            }
        }
    }

    Text {
        text: "logging=" + logging.checked + "  verbose=" + verbose.checked + "  colors=" + colors.checked
        color: "#7ee0ff"
    }
}
