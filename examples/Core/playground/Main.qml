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

    // Loop the offset from 0 to 2*PI infinitely
    NumberAnimation {
        target: rootWindow
        property: "gradientOffset"
        from: 0.0
        to: 2.0 * Math.PI
        duration: 2000
        loops: -1
        running: true
    }

    // Helper to generate ANSI truecolor escape codes for character-by-character color gradient
    function makeGradientText(rawText, offset) {
        let result = "";
        for (let i = 0; i < rawText.length; i++) {
            let char = rawText[i];
            if (char === " ") {
                result += " ";
                continue;
            }
            
            // Generate oscillating RGB values using phase-shifted sine waves
            let ratio = i / rawText.length;
            let r = Math.floor(127 + 128 * Math.sin(ratio * 2.0 * Math.PI + offset));
            let g = Math.floor(127 + 128 * Math.sin(ratio * 2.0 * Math.PI + offset + (2.0 * Math.PI / 3.0)));
            let b = Math.floor(127 + 128 * Math.sin(ratio * 2.0 * Math.PI + offset + (4.0 * Math.PI / 3.0)));
            
            // Format character with \u001b[38;2;R;G;Bm and reset sequence
            result += "\u001b[38;2;" + r + ";" + g + ";" + b + "m" + char + "\u001b[0m";
        }
        return result;
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

            // Animated Gradient Text component
            Text {
                width: parent.width
                textFormat: Text.AnsiText
                bold: true
                // Dynamic binding to makeGradientText, triggered on gradientOffset changes
                text: makeGradientText("MOVING COLOR GRADIENT TEXT DEMO WITH TRUECOLOR ANSI", rootWindow.gradientOffset)
            }

            Text {
                text: "Press 'q' or 'Escape' to quit."
                color: "gray"
            }
        }
    }
}

