import QtQml 2.15
import QtQml.Models 2.15
import Tui 1.0

Terminal {
    id: rootWindow
    fillTerminal: true
    backgroundColor: "black"
    useAlternateScreen: true

    property int score: 0
    property bool gameOver: false
    property int tickCounter: 0

    // This Timer is a standard Qt property, bypassing the `data` default property restrictions of Terminal
    Timer {
        id: gameTimer
        interval: 50
        running: true
        repeat: true
        onTriggered: {
            if (rootWindow.gameOver) return;

            rootWindow.tickCounter++;

            // Move bullets
            for (let i = bulletsModel.count - 1; i >= 0; i--) {
                let currentY = bulletsModel.get(i).yPos;
                if (currentY > 0) {
                    bulletsModel.setProperty(i, "yPos", currentY - 1);
                } else {
                    bulletsModel.remove(i);
                }
            }

            // Spawn enemies
            if (Math.random() < 0.1) {
                enemiesModel.append({
                    "xPos": Math.floor(Math.random() * (gameArea.width - 2)) + 1,
                    "yPos": 1
                });
            }

            // Move enemies
            if (rootWindow.tickCounter % 2 === 0) {
                for (let i = enemiesModel.count - 1; i >= 0; i--) {
                    let ex = enemiesModel.get(i).xPos;
                    let ey = enemiesModel.get(i).yPos;
                    ey += 1;
                    enemiesModel.setProperty(i, "yPos", ey);

                    // Check player collision
                    if (ex >= player.x && ex < player.x + player.width &&
                        ey >= player.y && ey < player.y + player.height) {
                        rootWindow.gameOver = true;
                    }

                    if (ey >= gameArea.height - 1) {
                        enemiesModel.remove(i);
                        rootWindow.score -= 5;
                    }
                }
            }

            // Collision detection (bullets vs enemies)
            for (let i = bulletsModel.count - 1; i >= 0; i--) {
                let bx = bulletsModel.get(i).xPos;
                let by = bulletsModel.get(i).yPos;
                let hit = false;

                for (let j = enemiesModel.count - 1; j >= 0; j--) {
                    let ex = enemiesModel.get(j).xPos;
                    let ey = enemiesModel.get(j).yPos;
                    // simple 1x1 AABB
                    if (bx === ex && by === ey) {
                        hit = true;
                        enemiesModel.remove(j);
                        rootWindow.score += 10;
                        break;
                    }
                }

                if (hit) {
                    bulletsModel.remove(i);
                }
            }
        }
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
            return
        }

        if (gameOver) {
            if (event.key === Qt.Key_R) {
                resetGame()
                event.accepted = true
            }
            return
        }

        if (event.key === Qt.Key_Left) {
            player.x = Math.max(1, player.x - 3)
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            player.x = Math.min(gameArea.width - player.width - 1, player.x + 3)
            event.accepted = true
        } else if (event.key === Qt.Key_Space) {
            shoot()
            event.accepted = true
        }
    }

    function resetGame() {
        score = 0
        gameOver = false
        player.x = Math.floor(gameArea.width / 2)
        bulletsModel.clear()
        enemiesModel.clear()
    }

    function shoot() {
        if (bulletsModel.count > 20) return;
        bulletsModel.append({
            "xPos": player.x + Math.floor(player.width / 2),
            "yPos": player.y - 1
        });
    }

    ListModel {
        id: bulletsModel
    }

    ListModel {
        id: enemiesModel
    }

    Rectangle {
        id: gameArea
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        border.style: "single"
        border.color: "gray"

        Text {
            text: " SCORE: " + rootWindow.score + " "
            color: "cyan"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            id: player
            text: "/A\\"
            color: "green"
            width: 3
            height: 1
            x: Math.floor(parent.width / 2)
            y: parent.height - 2
        }

        Repeater {
            model: bulletsModel
            delegate: Bullet {
                x: xPos
                y: yPos
            }
        }

        Repeater {
            model: enemiesModel
            delegate: Enemy {
                x: xPos
                y: yPos
            }
        }

        Text {
            visible: rootWindow.gameOver
            text: "GAME OVER"
            color: "red"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            visible: rootWindow.gameOver
            text: "Press 'R' to Restart"
            color: "white"
            anchors.top: parent.verticalCenter
            anchors.topMargin: 1
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            visible: rootWindow.gameOver
            text: "Press 'Q' to Quit"
            color: "gray"
            anchors.top: parent.verticalCenter
            anchors.topMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
