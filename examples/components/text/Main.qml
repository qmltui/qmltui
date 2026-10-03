import QtQml 2.15
import Qtui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
   
    color: "green"
    border.style: "single"
    border.color: "yellow"

    // Animation offset property
    property real gradientOffset: 0.0

    // Loop the offset from 0.0 to 1.0 infinitely
    NumberAnimation {
        target: rootWindow
        property: "gradientOffset"
        from: 0.0
        to: 1.0
        duration: 2000
        loops: -1
        running: true
    }

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 2
        anchors.bottomMargin: 1
        anchors.rightMargin: 2
        color: "#111111"
        border.style: "double"
        border.color: "gray"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Text {
                text: "🚀 Emoji & Animation Support in Qtui 🌟"
                color: "cyan"
                bold: true
            }

            Text {
                text: "Standard emojis:   😀 😎 🚀 🎉 🎯 🍕"
            }

            Text {
                text: "Country Flags:     🇺🇸 🇫🇷 🇩🇪 🇯🇵 🇨🇦 🇧🇷"
            }

            Text {
                text: "Keycaps & Hands:   1️⃣ 2️⃣ 👍 👍🏽 👏"
            }

            // Animated Gradient Text component using declarative Qt Quick-style Gradient API
            Text {
                width: parent.width
                bold: true
                text: "MOVING COLOR GRADIENT TEXT DEMO WITH TRUECOLOR ANSI"
                gradient: Gradient {
                    offset: rootWindow.gradientOffset
                    GradientStop { position: 0.0; color: "magenta" }
                    GradientStop { position: 0.33; color: "cyan" }
                    GradientStop { position: 0.66; color: "yellow" }
                    GradientStop { position: 1.0; color: "magenta" }
                }
                }

            Text {
                text: "Press 'q' or 'Escape' to quit."
                color: "gray"
            }
        }
    }
}

