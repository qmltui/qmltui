pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls
import Qtui.Image

// AnimatedSprite: Image plus a frame clock. One tile of the Qtui gallery
// (examples/apps/gallery) and a complete app on its own. The qami sheet is an
// 8x9 grid of 105x114 px cells and every animation is a run of cells in one
// row, so picking an animation is only new frameX/frameY/frameCount values.
// Row 5 holds two runs (sleep, then faint), which is why an animation also
// carries the column it starts at.
Terminal {
    id: root
    color: "#0b0f14"

    readonly property int cellWidth: 105
    readonly property int cellHeight: 114
    readonly property var animations: [
        { name: "idle", row: 0, firstCol: 0, frameCount: 6 },
        { name: "walk right", row: 1, firstCol: 0, frameCount: 8 },
        { name: "walk left", row: 2, firstCol: 0, frameCount: 8 },
        { name: "wave", row: 3, firstCol: 0, frameCount: 4 },
        { name: "jump", row: 4, firstCol: 0, frameCount: 5 },
        { name: "sleep", row: 5, firstCol: 0, frameCount: 5 },
        { name: "faint", row: 5, firstCol: 5, frameCount: 3 },
        { name: "dizzy", row: 6, firstCol: 0, frameCount: 6 },
        { name: "dance", row: 7, firstCol: 0, frameCount: 6 },
        { name: "cube", row: 8, firstCol: 0, frameCount: 6 }
    ]
    property int animationIndex: 0
    readonly property var animation: animations[animationIndex]

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        width: Math.min(60, root.width - 4)
        Text { text: "Left/Right pick the animation, Space pauses, R plays it backwards."; color: "#6e7b8b" }
        // The keys are handled on the panel, which takes focus, so they also
        // work when this file is hosted inside the gallery.
        Panel {
            width: parent.width
            height: 14
            title: "AnimatedSprite  ·  " + root.animation.name
            focusable: true
            focus: true
            Keys.onPressed: event => {
                const count = root.animations.length
                if (event.key === Qt.Key_Right)
                    root.animationIndex = (root.animationIndex + 1) % count
                else if (event.key === Qt.Key_Left)
                    root.animationIndex = (root.animationIndex + count - 1) % count
                else if (event.key === Qt.Key_Space)
                    sprite.paused = !sprite.paused
                else if (event.key === Qt.Key_R)
                    sprite.reverse = !sprite.reverse
                else
                    return
                event.accepted = true
            }
            // Braille, as in the Image tile: it keeps the figure's outline at
            // this size and reads well in the browser page too.
            AnimatedSprite {
                id: sprite
                anchors.fill: parent
                anchors.margins: 1
                anchors.topMargin: 2
                source: "qrc:/gallery/assets/qami.png"
                fillMode: Image.PreserveAspectFit
                renderMode: Image.Auto
                smooth: false
                frameX: root.animation.firstCol * root.cellWidth
                frameY: root.animation.row * root.cellHeight
                frameWidth: root.cellWidth
                frameHeight: root.cellHeight
                frameCount: root.animation.frameCount
                frameDuration: 180
            }
        }
        Text {
            text: root.animation.name + ": frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount
                  + (sprite.paused ? "   paused" : "") + (sprite.reverse ? "   reverse" : "")
            color: "#7ee0ff"
        }
        Text {
            text: sprite.status === Image.Ready
                  ? "Frame " + sprite.frameWidth + "x" + sprite.frameHeight + " px at (" + sprite.frameX + ", " + sprite.frameY + ") of a "
                    + sprite.sourceSize.width + "x" + sprite.sourceSize.height + " sheet"
                  : sprite.status === Image.Error ? "Sprite sheet failed to load" : "Loading"
            color: "#65e6a0"
            bold: true
        }
    }
}
