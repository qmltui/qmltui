pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Animation: NumberAnimation, PropertyAnimation, and ColorAnimation timelines with Easing curves.
Column {
    anchors.centerIn: parent
    width: 44
    spacing: 1

    Text {
        text: "NumberAnimation, PropertyAnimation, and ColorAnimation with Easing curves."
        color: "#8b949e"
    }

    Text {
        text: "Easing.OutBounce (NumberAnimation)"
        color: "#ff7b72"
    }

    Rectangle {
        id: bouncer
        width: 8
        height: 2
        color: "#ff7b72"

        NumberAnimation {
            id: bounceAnim
            target: bouncer
            property: "width"
            from: 8
            to: 44
            duration: 1600
            loops: -1
            running: true
            easing: Easing.OutBounce
        }
    }

    Text {
        text: "Easing.InOutQuad (PropertyAnimation)"
        color: "#58a6ff"
    }

    Rectangle {
        id: glider
        width: 8
        height: 2
        color: "#58a6ff"

        PropertyAnimation {
            id: quadAnim
            target: glider
            property: "width"
            from: 44
            to: 8
            duration: 1600
            loops: -1
            running: true
            easing: Easing.InOutQuad
        }
    }

    Text {
        text: "ColorAnimation (Green → Blue)"
        color: "#2cde85"
    }

    Rectangle {
        id: pulse
        width: 44
        height: 2
        color: "#2cde85"

        ColorAnimation {
            id: colorAnim
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

    Button {
        text: bounceAnim.running ? "Stop Animations" : "Start Animations"
        focus: true
        onClicked: {
            if (bounceAnim.running) {
                bounceAnim.stop();
                quadAnim.stop();
                colorAnim.stop();
            } else {
                bounceAnim.start();
                quadAnim.start();
                colorAnim.start();
            }
        }
    }
}
