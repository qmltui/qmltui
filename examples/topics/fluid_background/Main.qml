pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    id: root

    // Keyboard navigation and quit path
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_P) {
            fluid.running = !fluid.running
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            root.charsetIndex = (root.charsetIndex + 1) % root.charsets.length
            fluid.charset = root.charsets[root.charsetIndex].chars
            event.accepted = true
        } else if (event.key === Qt.Key_C) {
            root.paletteIndex = (root.paletteIndex + 1) % root.palettes.length
            const pal = root.palettes[root.paletteIndex]
            fluid.color = pal.c1
            fluid.secondaryColor = pal.c2
            fluid.colorMode = pal.mode
            event.accepted = true
        } else if (event.key === Qt.Key_W) {
            // Inject random wake
            fluid.addWake(Math.random(), Math.random(), 1.0)
            event.accepted = true
        }
    }

    // Available character sets
    readonly property var charsets: [
        { name: "Standard ASCII", chars: " ·:;=+*#%@" },
        { name: "Fine Dither",   chars: " ·∙•○◍●" },
        { name: "Terminal Blocks", chars: " ░▒▓█" },
        { name: "Unicode Braille", chars: " ⠁⠃⠇⠧⠷⠿" },
        { name: "Binary Stream",  chars: " 01" }
    ]
    property int charsetIndex: 0

    // Available color palettes
    readonly property var palettes: [
        { name: "Cyan / Pink (Asciify)", c1: "#38bdf8", c2: "#ec4899", mode: FluidBackground.Gradient },
        { name: "Neon Emerald / Amber",  c1: "#34d399", c2: "#fbbf24", mode: FluidBackground.Gradient },
        { name: "Cyber Purple / Aqua",   c1: "#a855f7", c2: "#06b6d4", mode: FluidBackground.Gradient },
        { name: "Monochrome Phosphor",   c1: "#22c55e", c2: "#22c55e", mode: FluidBackground.Accent },
        { name: "Grayscale High-Contrast", c1: "#ffffff", c2: "#ffffff", mode: FluidBackground.Monochrome }
    ]
    property int paletteIndex: 0

    // Fullscreen procedural fluid background
    FluidBackground {
        id: fluid
        anchors.fill: parent
        speed: 1.0
        charset: root.charsets[root.charsetIndex].chars
        color: root.palettes[root.paletteIndex].c1
        secondaryColor: root.palettes[root.paletteIndex].c2
        colorMode: root.palettes[root.paletteIndex].mode
        interactive: true
        autoWakes: true
    }

    // Centered HUD card
    Rectangle {
        id: hud
        anchors.centerIn: parent
        width: 54
        height: 12
        color: "#0f172a"
        border.style: Border.Double
        border.color: fluid.color

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: fluid.color
                bold: true
                text: "─── ASCIIFY FLUID BACKGROUND ───"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#94a3b8"
                text: "Procedural domain-warping fluid field"
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#1e293b"
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2
                Text { color: "#64748b"; text: "Charset:" }
                Text { color: "#f8fafc"; bold: true; text: root.charsets[root.charsetIndex].name }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2
                Text { color: "#64748b"; text: "Palette:" }
                Text { color: fluid.secondaryColor; bold: true; text: root.palettes[root.paletteIndex].name }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2
                Text { color: "#64748b"; text: "Status:" }
                Text {
                    color: fluid.running ? "#4ade80" : "#f87171"
                    text: fluid.running ? "PLAYING (25 FPS)" : "PAUSED"
                    bold: true
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#1e293b"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#e2e8f0"
                text: "[Space] Pause | [S] Charset | [C] Color | [W] Wake"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#64748b"
                text: "Click/drag mouse to stir wakes · [Q/ESC] to quit"
            }
        }
    }
}
