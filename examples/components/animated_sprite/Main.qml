pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Image

// AnimatedSprite plays a frame grid out of one sprite sheet. The fox sheet is
// a standard 8x9 grid of 192x208 cells; each animation is one row of it, so
// switching animations is just new frameX/frameY/frameCount values. Frames
// keep the full cell width and trim the cell to 198 rows with a 5 px top pad,
// the sprite's vertical alignment across the sheet.
Terminal {
    id: root
    color: "#0f172a"

    readonly property int cellWidth: 192
    readonly property int cellHeight: 208
    readonly property int frameTopPad: 5

    readonly property var animations: [
        {
            name: "idle",
            row: 0,
            firstCol: 0,
            frameCount: 4
        },
        {
            name: "wave",
            row: 3,
            firstCol: 0,
            frameCount: 4
        },
        {
            name: "jump",
            row: 4,
            firstCol: 0,
            frameCount: 4
        },
        {
            name: "sleep",
            row: 5,
            firstCol: 2,
            frameCount: 3
        }
    ]
    property int animationIndex: 0
    readonly property var animation: animations[animationIndex]

    readonly property string activePath: capabilities.kittyGraphics ? "kitty (real pixels)" : capabilities.sixelGraphics ? "sixel (real pixels)" : "half-block fallback"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_4) {
            root.animationIndex = event.key - Qt.Key_1;
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            fox.paused = !fox.paused;
            event.accepted = true;
        } else if (event.key === Qt.Key_R) {
            fox.reverse = !fox.reverse;
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            width: parent.width
            text: "AnimatedSprite  —  a fox out of one sprite sheet"
            color: "cyan"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
        }
        Text {
            width: parent.width
            text: "[1-4] animation  [Space] pause  [R] reverse  [Q] quit"
            color: "#94a3b8"
            horizontalAlignment: Qt.AlignHCenter
        }

        Row {
            height: 1
            spacing: 1
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: root.animations
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: modelData.name.length + 4
                    height: 1
                    color: root.animationIndex === index ? "#0284c7" : "#1e293b"

                    Text {
                        anchors.centerIn: parent
                        text: (root.animationIndex === index ? "▶ " : "  ") + modelData.name
                        color: root.animationIndex === index ? "white" : "#94a3b8"
                        bold: root.animationIndex === index
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height - 10
            color: "#111827"
            border.style: "single"
            border.color: "#334155"

            AnimatedSprite {
                id: fox
                anchors.centerIn: parent
                height: 4
                width: 8
                fillMode: Image.PreserveAspectFit
                renderMode: Image.Auto
                source: "fox_spritesheet.webp"
                frameX: root.animation.firstCol * root.cellWidth
                frameY: root.animation.row * root.cellHeight + root.frameTopPad
                frameWidth: root.cellWidth
                frameHeight: root.cellHeight - 2 * root.frameTopPad
                frameCount: root.animation.frameCount
                frameDuration: 180
            }
        }

        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "yellow"
            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "Animation: " + root.animation.name + "   frame " + (fox.currentFrame + 1) + "/" + fox.frameCount + (fox.paused ? "   paused" : "") + (fox.reverse ? "   reverse" : "") + "   Path: " + root.activePath
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
