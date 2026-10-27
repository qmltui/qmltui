pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Repeater: Instantiates one delegate per entry from an integer count or model array.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Repeater creates one delegate per model entry inside a positioner."
        color: "#8b949e"
    }

    Text {
        text: "Count: " + countSlider.value
        color: "#c9d1d9"
    }

    Slider {
        id: countSlider
        width: 30
        from: 1
        to: 20
        value: 7
        focus: true
    }

    // Integer model: instantiates `countSlider.value` delegates
    Row {
        spacing: 0

        Repeater {
            model: countSlider.value
            delegate: Text {
                required property int index
                text: "▮"
                color: index < 10 ? "#2cde85" : "#58a6ff"
            }
        }
    }

    // String array model: exposes `modelData` to each delegate
    Row {
        spacing: 1

        Repeater {
            model: ["alpha", "beta", "gamma", "delta"]
            delegate: Rectangle {
                required property string modelData
                width: modelData.length + 4
                height: 1
                color: "#122d24"

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: "#2cde85"
                }
            }
        }
    }
}
