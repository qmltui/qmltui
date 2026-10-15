pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls
import QmlTui.Image

// AnimatedSprite: Image plus a frame clock. One tile of the QmlTui gallery
// (examples/gallery) and a complete app on its own. The loading sheet is
// one row of four 64x64 px cells and an animation is a run of cells in that
// row, so picking an animation is only new frameX/frameY/frameCount values;
// the second entry plays the last two cells to show that the run can start
// anywhere.
Terminal {
    id: root
    color: "#0d1117"

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

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        width: Math.min(60, root.width - 4)
        Text {
            text: "Left/Right pick the animation, Space pauses, R plays it backwards."
            color: "#8b949e"
        }
        // The keys are handled on the panel, which takes focus, so they also
        // work when this file is hosted inside the gallery.
        Panel {
            width: parent.width
            height: 14
            title: "AnimatedSprite  ·  " + root.animation.name
            focusable: true
            focus: true
            Keys.onPressed: event => {
                const count = root.animations.length;
                if (event.key === Qt.Key_Right)
                    root.animationIndex = (root.animationIndex + 1) % count;
                else if (event.key === Qt.Key_Left)
                    root.animationIndex = (root.animationIndex + count - 1) % count;
                else if (event.key === Qt.Key_Space)
                    sprite.paused = !sprite.paused;
                else if (event.key === Qt.Key_R)
                    sprite.reverse = !sprite.reverse;
                else
                    return;
                event.accepted = true;
            }
            // Braille, as in the Image tile: it keeps the figure's outline at
            // this size and reads well in the browser page too.
            AnimatedSprite {
                id: sprite
                anchors.fill: parent
                anchors.margins: 1
                anchors.topMargin: 2
                source: "qrc:/gallery/assets/loading.png"
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
            text: root.animation.name + ": frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount + (sprite.paused ? "   paused" : "") + (sprite.reverse ? "   reverse" : "")
            color: "#7ee0ff"
        }
        Text {
            text: sprite.status === Image.Ready ? "Frame " + sprite.frameWidth + "x" + sprite.frameHeight + " px at (" + sprite.frameX + ", " + sprite.frameY + ") of a " + sprite.sourceSize.width + "x" + sprite.sourceSize.height + " sheet" : sprite.status === Image.Error ? "Sprite sheet failed to load" : "Loading"
            color: "#2cde85"
            bold: true
        }
    }
}
