pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Image

// AnimatedSprite: Plays a sequence of sub-frames sliced out of a sprite sheet.
Column {
    id: spriteDemo

    readonly property int cellWidth: 64
    readonly property int cellHeight: 64
    readonly property var animations: [
        {
            name: "loading",
            row: 0,
            firstCol: 0,
            frameCount: 4
        },
        {
            name: "loading tail",
            row: 0,
            firstCol: 2,
            frameCount: 2
        }
    ]
    property int animationIndex: 0
    readonly property var animation: animations[animationIndex]

    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Keys.onPressed: event => {
        const count = spriteDemo.animations.length;
        if (event.key === Qt.Key_Right)
            spriteDemo.animationIndex = (spriteDemo.animationIndex + 1) % count;
        else if (event.key === Qt.Key_Left)
            spriteDemo.animationIndex = (spriteDemo.animationIndex + count - 1) % count;
        else if (event.key === Qt.Key_Space)
            sprite.paused = !sprite.paused;
        else if (event.key === Qt.Key_R)
            sprite.reverse = !sprite.reverse;
        else
            return;
        event.accepted = true;
    }

    Text {
        text: "Left/Right switch clip, Space pauses, R reverses."
        color: "#8b949e"
    }

    Rectangle {
        width: Math.min(44, parent.width)
        height: 11
        border {
            style: Border.Single
            color: "#30363d"
        }

        // Slice frameCount cells of size frameWidth×frameHeight starting at (frameX, frameY)
        AnimatedSprite {
            id: sprite
            anchors {
                fill: parent
                margins: 1
            }
            source: "qrc:/gallery/assets/loading.png"
            fillMode: Image.PreserveAspectFit
            renderMode: Image.Auto
            smooth: false
            frameX: spriteDemo.animation.firstCol * spriteDemo.cellWidth
            frameY: spriteDemo.animation.row * spriteDemo.cellHeight
            frameWidth: spriteDemo.cellWidth
            frameHeight: spriteDemo.cellHeight
            frameCount: spriteDemo.animation.frameCount
            frameDuration: 180
        }
    }

    Text {
        text: spriteDemo.animation.name + ": frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount + (sprite.paused ? "   paused" : "") + (sprite.reverse ? "   reverse" : "")
        color: "#7ee0ff"
    }

    Text {
        text: sprite.status === Image.Ready ? "Frame " + sprite.frameWidth + "x" + sprite.frameHeight + " px of " + sprite.sourceSize.width + "x" + sprite.sourceSize.height + " sheet" : "Loading"
        color: "#2cde85"
        bold: true
    }
}
