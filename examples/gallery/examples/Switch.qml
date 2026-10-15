pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Switch: On/off with a label. One tile of the QmlTui gallery (examples/gallery)
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
            text: "Switches read as [ ON] / [OFF]."
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
}
