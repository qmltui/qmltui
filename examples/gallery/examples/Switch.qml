pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Switch: Binary on/off toggle rendered as [ ON] / [OFF].
Column {
    spacing: 1

    Text {
        text: "Space, Enter, or click toggles a Switch between [ ON] and [OFF]."
        color: "#8b949e"
    }

    Switch {
        id: wifi
        text: "Wi-Fi"
        checked: true
        focus: true
    }

    Switch {
        id: bluetooth
        text: "Bluetooth"
    }

    Switch {
        id: airplane
        text: "Airplane mode"
        textColor: "#ff9f6e"
    }

    Text {
        text: airplane.checked ? "Radios are off while airplane mode is on" : "Wi-Fi " + (wifi.checked ? "on" : "off") + ", Bluetooth " + (bluetooth.checked ? "on" : "off")
        color: "#ff9f6e"
    }
}
