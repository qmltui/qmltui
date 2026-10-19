pragma ComponentBehavior: Bound
import QmlTui

// Animation: NumberAnimation and ColorAnimation timelines with easing curves.
Column {
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
            color: "#1e293b"
        }

        Text {
            id: bouncer
            text: "████"
            color: "#f9a8d4"

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
            color: "#1e293b"
        }

        Text {
            id: glider
            text: "◆◆◆◆"
            color: "#c084fc"

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
        color: "#f9a8d4"

        ColorAnimation {
            target: pulse
            property: "color"
            from: "#f9a8d4"
            to: "#312e81"
            duration: 1600
            loops: -1
            running: true
            easing: Easing.InOutQuad
        }
    }

    Text {
        text: "x = " + bouncer.x + " / " + glider.x
        color: "#f9a8d4"
    }
}
