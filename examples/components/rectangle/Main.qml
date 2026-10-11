pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    id: root

    property int borderStyleIndex: 0
    property var borderStyles: ["single", "double", "ascii", "none"]
    property bool isVertical: true
    property real animOffset: 0.0
    property bool animRunning: true

    NumberAnimation {
        id: flowAnimation
        target: root
        property: "animOffset"
        from: 0.0
        to: 1.0
        duration: 3000
        loops: -1
        running: root.animRunning
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            root.animRunning = !root.animRunning;
            event.accepted = true;
        } else if (event.key === Qt.Key_B) {
            root.borderStyleIndex = (root.borderStyleIndex + 1) % root.borderStyles.length;
            event.accepted = true;
        } else if (event.key === Qt.Key_O) {
            root.isVertical = !root.isVertical;
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Header Banner with continuously animated horizontal flow
        Rectangle {
            width: parent.width
            height: 3
            border.style: "double"
            border.color: "cyan"
            gradient: Gradient {
                orientation: Gradient.Horizontal
                offset: root.animOffset
                GradientStop { position: 0.0; color: "#0f2027" }
                GradientStop { position: 0.33; color: "#203a43" }
                GradientStop { position: 0.66; color: "#2c5364" }
                GradientStop { position: 1.0; color: "#0f2027" }
            }

            Text {
                anchors.centerIn: parent
                text: "✨ Qamui Rectangle Showcase — Borders, Colors & Animated Gradients ✨"
                bold: true
                color: "white"
            }
        }

        // Section 1: Border Styles
        Row {
            spacing: 2

            Rectangle {
                width: 20
                height: 5
                border.style: "single"
                border.color: "cyan"

                Column {
                    id: content
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "Single Border"; bold: true; color: "cyan" }
                    Text { text: 'style: "single"'; dim: true }
                }
            }

            Rectangle {
                width: 20
                height: 5
                border.style: "double"
                border.color: "yellow"

                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "Double Border"; bold: true; color: "yellow" }
                    Text { text: 'style: "double"'; dim: true }
                }
            }

            Rectangle {
                width: 20
                height: 5
                border.style: "ascii"
                border.color: "lime"

                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "ASCII Border"; bold: true; color: "lime" }
                    Text { text: 'style: "ascii"'; dim: true }
                }
            }

            Rectangle {
                width: 20
                height: 5
                border.style: "none"
                color: "#2e3440"

                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "No Border"; bold: true; color: "white" }
                    Text { text: 'style: "none"'; dim: true }
                }
            }
        }

        // Section 2: Animated Gradients & Nested Content
        Row {
            spacing: 2

            // Ocean Wave animated gradient card
            Rectangle {
                width: 23
                height: 7

                gradient: Gradient {
                    orientation: root.isVertical ? Gradient.Vertical : Gradient.Horizontal
                    offset: root.animOffset
                    GradientStop { position: 0.0; color: "#1e3c72" }
                    GradientStop { position: 0.5; color: "#2a5298" }
                    GradientStop { position: 1.0; color: "#1e3c72" }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "🌊 Ocean Wave"; bold: true; color: "white" }
                    Text { text: root.isVertical ? "Vertical Shading" : "Horizontal Shading"; dim: true }
                    Text { text: "Offset: " + root.animOffset.toFixed(2); color: "cyan" }
                }
            }

            // Cyber Sunset animated gradient card
            Rectangle {
                width: 23
                height: 7

                gradient: Gradient {
                    orientation: root.isVertical ? Gradient.Vertical : Gradient.Horizontal
                    offset: (root.animOffset + 0.5) % 1.0
                    GradientStop { position: 0.0; color: "#8a2387" }
                    GradientStop { position: 0.5; color: "#e94057" }
                    GradientStop { position: 1.0; color: "#8a2387" }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 0
                    Text { text: "🔥 Cyber Glow"; bold: true; color: "white" }
                    Text { text: "Phase Shift (+0.5)"; dim: true }
                    Text { text: root.animRunning ? "Status: Flowing" : "Status: Paused"; color: "yellow" }
                }
            }

            // Interactive dynamic card with nested badge
            Rectangle {
                width: 24
                height: 7
                border.style: root.borderStyles[root.borderStyleIndex]
                border.color: "yellow"
                gradient: Gradient {
                    orientation: root.isVertical ? Gradient.Vertical : Gradient.Horizontal
                    offset: (1.0 - root.animOffset)
                    GradientStop { position: 0.0; color: "#11998e" }
                    GradientStop { position: 0.5; color: "#38ef7d" }
                    GradientStop { position: 1.0; color: "#11998e" }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 0

                    Text {
                        text: "✨ Reverse Flow";
                        bold: true
                        color: "#001b10"
                    }

                    // Nested badge with its own solid background and border
                    Rectangle {
                        width: 18
                        height: 3
                        border.style: "single"
                        border.color: "#001b10"
                        color: "#001b10"

                        Text {
                            anchors.centerIn: parent
                            text: "Nested Badge"
                            color: "#38ef7d"
                            bold: true
                        }
                    }
                }
            }
        }

        // Footer / Interactive Controls HUD
        Rectangle {
            width: parent.width
            height: 3
            border.style: "single"
            border.color: "#585b70"
            color: "#181825"

            Row {
                anchors.centerIn: parent
                spacing: 3

                Text {
                    text: "[Space] " + (root.animRunning ? "Pause ⏸" : "Play ▶")
                    color: root.animRunning ? "lime" : "yellow"
                }

                Text { text: "•"; dim: true }

                Text {
                    text: "[B] Border: " + root.borderStyles[root.borderStyleIndex]
                    color: "cyan"
                }

                Text { text: "•"; dim: true }

                Text {
                    text: "[O] Orientation: " + (root.isVertical ? "Vertical" : "Horizontal")
                    color: "orange"
                }

                Text { text: "•"; dim: true }

                Text {
                    text: "[Q / Esc] Quit"
                    color: "magenta"
                }
            }
        }
    }
}
