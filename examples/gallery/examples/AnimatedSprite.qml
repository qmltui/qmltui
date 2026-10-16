pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import QmlTui.Image

Item {
    id: root
    property int animIndex: 0
    readonly property var animations: [
        {
            name: "loading",
            frameX: 0,
            frameY: 0,
            frameCount: 4
        },
        {
            name: "loading tail",
            frameX: 128,
            frameY: 0,
            frameCount: 2
        }
    ]
    readonly property var animation: animations[animIndex]

    function switchAnimation(delta) {
        root.animIndex = (root.animIndex + delta + root.animations.length) % root.animations.length;
        sprite.restart();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                sprite.paused = !sprite.paused;
                event.accepted = true;
            } else if (event.key === Qt.Key_Right) {
                root.switchAnimation(1);
                event.accepted = true;
            } else if (event.key === Qt.Key_Left) {
                root.switchAnimation(-1);
                event.accepted = true;
            } else if (event.key === Qt.Key_R) {
                sprite.reverse = !sprite.reverse;
                event.accepted = true;
            }
        }

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            title: "AnimatedSprite"
            titleColor: "#fdba74"
            titleBadge: sprite.paused ? "[PAUSED]" : "[PLAYING]"
            titleBadgeColor: sprite.paused ? "#f9c74f" : "#2cde85"
            titleTrailing: (sprite.currentFrame + 1) + "/" + sprite.frameCount
            titleTrailingColor: "#58a6ff"
            footer: "[Space] Pause  [Left/Right] Clip  [R] Reverse"
            footerColor: "#8b949e"
            footerTrailing: "220 ms/f"
            footerTrailingColor: "#fdba74"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: root.animation.name + ": frame " + (sprite.currentFrame + 1) + "/" + sprite.frameCount + (sprite.paused ? "   paused" : "") + (sprite.reverse ? "   reverse" : "")
                    bold: true
                    color: "#fdba74"
                }

                Text {
                    text: "x=" + (sprite.frameX + sprite.currentFrame * sprite.frameWidth) + "..+" + sprite.frameWidth + " of a 256x64 sheet"
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 12
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    AnimatedSprite {
                        id: sprite
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        source: "qrc:/gallery/assets/loading.png"
                        frameX: root.animation.frameX
                        frameY: root.animation.frameY
                        frameWidth: 64
                        frameHeight: 64
                        frameCount: root.animation.frameCount
                        frameDuration: 220
                        fillMode: Image.PreserveAspectFit
                        renderMode: Image.HalfBlock
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "clip: " + root.animation.name
                    bold: true
                    color: "#fdba74"
                }

                Text {
                    text: "frameX/Y: " + sprite.frameX + ", " + sprite.frameY
                    color: "#c9d1d9"
                }

                Text {
                    text: "frameSize: " + sprite.frameWidth + "x" + sprite.frameHeight
                    color: "#2cde85"
                }

                Text {
                    text: "frameDuration: " + sprite.frameDuration + " ms"
                    color: "#58a6ff"
                }

                Text {
                    text: "paused: " + sprite.paused
                    color: sprite.paused ? "#f9c74f" : "#8b949e"
                }

                Text {
                    text: "reverse: " + sprite.reverse
                    color: sprite.reverse ? "#f472b6" : "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: sprite.paused ? "Resume [Space]" : "Pause [Space]"
                    onClicked: sprite.paused = !sprite.paused
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Next Clip [Right]"
                    onClicked: root.switchAnimation(1)
                }
            }
        }
    }
}
