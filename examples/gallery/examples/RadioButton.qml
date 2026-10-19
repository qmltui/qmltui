pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// RadioButton: Mutually exclusive choices coordinated by a ButtonGroup.
Column {
    spacing: 1

    Text {
        text: "ButtonGroup keeps only one RadioButton checked at a time."
        color: "#8b949e"
    }

    // Group the radio buttons so checking one unchecks the others
    ButtonGroup {
        buttons: [fast, balanced, safe]
    }

    Row {
        spacing: 2

        RadioButton {
            id: fast
            text: "Fast"
            checked: true
            focus: true
        }

        RadioButton {
            id: balanced
            text: "Balanced"
        }

        RadioButton {
            id: safe
            text: "Safe"
        }
    }

    Text {
        text: "Profile: " + (fast.checked ? "fast (no fsync)" : balanced.checked ? "balanced" : "safe (fsync every write)")
        color: "#f9c74f"
    }
}
