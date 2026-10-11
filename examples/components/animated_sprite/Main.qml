pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Image

// AnimatedSprite example based on the Qt Quick documentation:
// Demonstrates a 4-frame loading animation from loading.png (256x64, 4 frames of 64x64).
Terminal {
    id: root
    color: "#0f172a"

    readonly property string activePath: capabilities.kittyGraphics
        ? "kitty (real pixels)"
        : capabilities.sixelGraphics
            ? "sixel (real pixels)"
            : "half-block fallback"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            sprite.paused = !sprite.paused;
            event.accepted = true;
        } else if (event.key === Qt.Key_R) {
            sprite.reverse = !sprite.reverse;
            event.accepted = true;
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            sprite.frameDuration = Math.max(100, sprite.frameDuration - 100);
            event.accepted = true;
        } else if (event.key === Qt.Key_Minus) {
            sprite.frameDuration = Math.min(2000, sprite.frameDuration + 100);
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Text {
            width: parent.width
            text: "AnimatedSprite  —  Loading sprite example"
            color: "cyan"
            bold: true
            horizontalAlignment: Qt.AlignHCenter
        }

        Text {
            width: parent.width
            text: "[Space] pause/resume  [R] reverse  [+/-] speed  [Q] quit"
            color: "#94a3b8"
            horizontalAlignment: Qt.AlignHCenter
        }

        Rectangle {
            width: parent.width
            height: Math.max(3, parent.height - 8)
            color: "#111827"
            border.style: "single"
            border.color: "#334155"

            AnimatedSprite {
                id: sprite
                objectName: "loadingSprite"
                anchors.centerIn: parent
                width: 24
                height: 10
                fillMode: Image.PreserveAspectFit
                source: "loading.png"
                frameWidth: 64
                frameHeight: 64
                frameCount: 4
                frameDuration: 500
                interpolate: false
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
                text: "Animation: loading   frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount
                      + (sprite.paused ? "   [paused]" : "   [running]")
                      + (sprite.reverse ? "   [reverse]" : "")
                      + "   duration: " + sprite.frameDuration + " ms"
                      + "\nPath: " + root.activePath
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
