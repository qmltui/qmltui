pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Slider: Value track adjusted via Left/Right arrow keys or mouse click/drag.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Left/Right step by stepSize; clicking snaps the handle."
        color: "#8b949e"
    }

    Text {
        text: "Volume  " + volume.value + "%"
        color: "#2cde85"
    }

    // Continuous 0..100 slider
    Slider {
        id: volume
        width: 36
        from: 0
        to: 100
        value: 42
        focus: true
    }

    Text {
        text: "Contrast  " + contrast.value
        color: "#2cde85"
    }

    // Signed range slider with explicit stepSize
    Slider {
        id: contrast
        width: 36
        from: -10
        to: 10
        value: 3
        stepSize: 1
    }

    // Visual bar bound to volume.value
    Rectangle {
        width: Math.max(1, Math.round(volume.value / 100 * 36))
        height: 1
        color: "#2cde85"
    }
}
