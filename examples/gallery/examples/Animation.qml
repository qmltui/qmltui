pragma ComponentBehavior: Bound
import QmlTui

// Animation: NumberAnimation and ColorAnimation timelines with easing curves.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "NumberAnimation and ColorAnimation loop with easing curves."
        color: "#8b949e"
    }

    Text {
        text: "Easing.OutBounce"
        color: "#c9d1d9"
    }

    Item {
        width: 40
        height: 1

        Text {
            text: "─".repeat(40)
            color: "#30363d"
        }

        Text {
            id: bouncer
            text: "████"
            color: "#ff7b72"

            NumberAnimation {
                target: bouncer
                property: "x"
                from: 0
                to: 36
                duration: 1600
                loops: -1
                running: true
                easing: Easing.OutBounce
            }
        }
    }

    Text {
        text: "Easing.InOutQuad"
        color: "#c9d1d9"
    }

    Item {
        width: 40
        height: 1

        Text {
            text: "─".repeat(40)
            color: "#30363d"
        }

        Text {
            id: glider
            text: "◆◆◆◆"
            color: "#58a6ff"

            NumberAnimation {
                target: glider
                property: "x"
                from: 36
                to: 0
                duration: 1600
                loops: -1
                running: true
                easing: Easing.InOutQuad
            }
        }
    }

    Text {
        id: pulse
        text: "ColorAnimation: fades between two colours"
        bold: true
        color: "#2cde85"

        ColorAnimation {
            target: pulse
            property: "color"
            from: "#2cde85"
            to: "#58a6ff"
            duration: 1600
            loops: -1
            running: true
            easing: Easing.InOutQuad
        }
    }

    Text {
        text: "x = " + bouncer.x + " / " + glider.x
        color: "#79c0ff"
    }
}
