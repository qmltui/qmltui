pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Image

// AnimatedSprite plays a frame grid out of one sprite sheet. The qami sheet is
// an 8x9 grid of 105x114 cells; each animation is a run of cells in one row,
// so switching animations is just new frameX/frameY/frameCount values. Row 5
// holds two runs (sleep, then faint), which is why an animation also carries
// the column it starts at.
Terminal {
    id: root
    color: "#0f172a"

    readonly property int cellWidth: 105
    readonly property int cellHeight: 114

    readonly property var animations: [
        {
            name: "idle",
            key: "1",
            row: 0,
            firstCol: 0,
            frameCount: 6
        },
        {
            name: "walk right",
            key: "2",
            row: 1,
            firstCol: 0,
            frameCount: 8
        },
        {
            name: "walk left",
            key: "3",
            row: 2,
            firstCol: 0,
            frameCount: 8
        },
        {
            name: "wave",
            key: "4",
            row: 3,
            firstCol: 0,
            frameCount: 4
        },
        {
            name: "jump",
            key: "5",
            row: 4,
            firstCol: 0,
            frameCount: 5
        },
        {
            name: "sleep",
            key: "6",
            row: 5,
            firstCol: 0,
            frameCount: 5
        },
        {
            name: "faint",
            key: "7",
            row: 5,
            firstCol: 5,
            frameCount: 3
        },
        {
            name: "dizzy",
            key: "8",
            row: 6,
            firstCol: 0,
            frameCount: 6
        },
        {
            name: "dance",
            key: "9",
            row: 7,
            firstCol: 0,
            frameCount: 6
        },
        {
            name: "cube",
            key: "0",
            row: 8,
            firstCol: 0,
            frameCount: 6
        }
    ]
    readonly property int tabsPerRow: 5
    property int animationIndex: 0
    readonly property var animation: animations[animationIndex]

    readonly property string activePath: capabilities.kittyGraphics ? "kitty (real pixels)" : capabilities.sixelGraphics ? "sixel (real pixels)" : "half-block fallback"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
            root.animationIndex = event.key - Qt.Key_1;
            event.accepted = true;
        } else if (event.key === Qt.Key_0) {
            root.animationIndex = 9;
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            root.animationIndex = (root.animationIndex + 1) % root.animations.length;
            event.accepted = true;
        } else if (event.key === Qt.Key_Left) {
            root.animationIndex = (root.animationIndex + root.animations.length - 1) % root.animations.length;
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            sprite.paused = !sprite.paused;
            event.accepted = true;
        } else if (event.key === Qt.Key_R) {
            sprite.reverse = !sprite.reverse;
            event.accepted = true;
        }
    }

    // One tab per animation. The Repeater initializes required properties on
    // the delegate root only, so each delegate declares them and hands the
    // values over here.
    component AnimationTab: Rectangle {
        property var animation
        property int slot: 0
        readonly property bool active: root.animationIndex === slot
        width: animation.name.length + 5
        height: 1
        color: active ? "#0284c7" : "#1e293b"

        Text {
            anchors.centerIn: parent
            text: (parent.active ? "▶" : parent.animation.key) + " " + parent.animation.name
            color: parent.active ? "white" : "#94a3b8"
            bold: parent.active
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            width: parent.width
            text: "AnimatedSprite  —  qami out of one sprite sheet"
            color: "cyan"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
        }
        Text {
            width: parent.width
            text: "[1-9,0] animation  [←/→] cycle  [Space] pause  [R] reverse  [Q] quit"
            color: "#94a3b8"
            horizontalAlignment: Qt.AlignHCenter
        }

        Column {
            width: parent.width
            spacing: 0

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                height: 1
                spacing: 1
                Repeater {
                    model: root.animations.slice(0, root.tabsPerRow)
                    delegate: AnimationTab {
                        required property var modelData
                        required property int index
                        animation: modelData
                        slot: index
                    }
                }
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                height: 1
                spacing: 1
                Repeater {
                    model: root.animations.slice(root.tabsPerRow)
                    delegate: AnimationTab {
                        required property var modelData
                        required property int index
                        animation: modelData
                        slot: index + root.tabsPerRow
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height - 11
            color: "#111827"
            border.style: "single"
            border.color: "#334155"

            AnimatedSprite {
                id: sprite
                anchors.centerIn: parent
                height: 4
                width: 8
                fillMode: Image.PreserveAspectFit
                renderMode: Image.Auto
                smooth: false
                source: "qami.png"
                frameX: root.animation.firstCol * root.cellWidth
                frameY: root.animation.row * root.cellHeight
                frameWidth: root.cellWidth
                frameHeight: root.cellHeight
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
                text: "Animation: " + root.animation.name + "   frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount + (sprite.paused ? "   paused" : "") + (sprite.reverse ? "   reverse" : "") + "   Path: " + root.activePath
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
