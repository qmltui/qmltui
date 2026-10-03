import QtQml 2.15
import Qtui
import Qtui.Controls

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Qtui Slider Component Example"
            color: "cyan"
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text { text: "Master Volume: " + volSlider.value + "%"; color: "yellow" }

        Slider {
            id: volSlider
            width: 36
            value: 75
            from: 0
            to: 100
        }

        Text { text: "Display Brightness: " + brightSlider.value + "%"; color: "yellow" }

        Slider {
            id: brightSlider
            width: 36
            value: 40
            from: 0
            to: 100
        }

        Text { text: "──────────────────────────────────────────"; color: "gray" }

        Text {
            text: "Use Left/Right or Mouse Drag to Adjust Sliders"
            color: "gray"
        }
    }
}
