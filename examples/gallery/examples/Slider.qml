pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Slider: Left/Right or drag. One tile of the QmlTui gallery (examples/gallery)
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
            text: "Left/Right step by stepSize; a click snaps the handle."
            color: "#8b949e"
        }
        Text {
            text: "Volume  " + volume.value + "%"
            color: "#c084fc"
        }
        Slider {
            id: volume
            width: 40
            from: 0
            to: 100
            value: 42
            focus: true
        }
        Text {
            text: "Contrast  " + contrast.value
            color: "#c084fc"
        }
        Slider {
            id: contrast
            width: 40
            from: -10
            to: 10
            value: 3
            stepSize: 1
        }
        Rectangle {
            width: Math.max(1, Math.round(volume.value / 100 * 40))
            height: 1
            color: "#c084fc"
        }
    }
}
