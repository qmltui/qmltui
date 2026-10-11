pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// Repeater: One delegate per entry. One tile of the Qamui gallery (examples/apps/gallery)
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
        Text { text: "A Repeater creates one delegate per model entry: a count, an array, or a model."; color: "#6e7b8b" }
        Text { text: "Count: " + countSlider.value; color: "#c9d1d9" }
        Slider { id: countSlider; width: 30; from: 1; to: 20; value: 7; focus: true }
        Row {
            spacing: 0
            Repeater {
                model: countSlider.value
                delegate: Text {
                    required property int index
                    text: "▮"
                    color: index < 10 ? "#c4b5fd" : "#f472b6"
                }
            }
        }
        Row {
            spacing: 2
            Repeater {
                model: ["alpha", "beta", "gamma", "delta"]
                delegate: Rectangle {
                    required property string modelData
                    width: modelData.length + 4
                    height: 1
                    color: "#2e1065"
                    Text { anchors.centerIn: parent; text: modelData; color: "#c4b5fd" }
                }
            }
        }
    }
}
