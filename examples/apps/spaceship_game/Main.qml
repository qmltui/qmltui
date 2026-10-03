import QtQml 2.15
import QtQml.Models 2.15
import Qtui

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "black"
    useAlternateScreen: true

    property int score: 0
    property int highScore: 0
    property int lives: 3
    property int health: 5
    property int maxHealth: 5
    property int level: 1
    property bool gameOver: false
    property int tickCounter: 0
    property int pendingXMove: 0
    property int pendingYMove: 0
    property int lastShootTick: 0

    // Power-up states
    property string fireMode: "single" // "single", "double", "triple"
    property int firePowerTicks: 0
    property int shieldTicks: 0
    property bool shieldActive: false

    // Reset game state
    function resetGame() {
        score = 0
        lives = 3
        health = 5
        level = 1
        gameOver = false
        fireMode = "single"
        firePowerTicks = 0
        shieldTicks = 0
        shieldActive = false
        player.x = Math.floor(gameArea.width / 2) - 1
        player.y = gameArea.height - 2
        pendingXMove = 0
        pendingYMove = 0
        lastShootTick = 0
        bulletsModel.clear()
        enemiesModel.clear()
        explosionsModel.clear()
        powerupsModel.clear()
        // Generate initial stars
        starsModel.clear()
        for (let i = 0; i < 20; i++) {
            starsModel.append({
                "xPos": Math.floor(Math.random() * (gameArea.width - 2)) + 1,
                "yPos": Math.floor(Math.random() * (gameArea.height - 2)) + 1,
                "speed": Math.floor(Math.random() * 3) + 1
            });
        }
    }

    // Shoot bullets
    function shoot() {
        if (gameOver) return;
        if (tickCounter - lastShootTick < 3) return; // Cooldown of 3 ticks (150ms)
        if (bulletsModel.count > 30) return;

        lastShootTick = tickCounter;

        let px = player.x + 1; // Center of player
        let py = player.y - 1;

        if (fireMode === "single") {
            bulletsModel.append({ "xPos": px, "yPos": py });
        } else if (fireMode === "double") {
            bulletsModel.append({ "xPos": px - 1, "yPos": py });
            bulletsModel.append({ "xPos": px + 1, "yPos": py });
        } else if (fireMode === "triple") {
            bulletsModel.append({ "xPos": px - 1, "yPos": py });
            bulletsModel.append({ "xPos": px, "yPos": py - 1 });
            bulletsModel.append({ "xPos": px + 1, "yPos": py });
        }
    }

    // Spawn an explosion
    function spawnExplosion(x, y) {
        explosionsModel.append({ "xPos": x, "yPos": y, "frame": 0 });
    }

    // Spawn a power-up
    function spawnPowerUp(x, y) {
        let rand = Math.random();
        let pType = "heal";
        if (rand < 0.35) {
            pType = "shield";
        } else if (rand < 0.7) {
            pType = "fire";
        }
        powerupsModel.append({ "xPos": x, "yPos": y, "type": pType });
    }

    // Triggered game loop
    Timer {
        id: gameTimer
        interval: 50 // Fast frame rate for fluid TUI animation
        running: true
        repeat: true
        onTriggered: {
            if (rootWindow.gameOver) return;

            // Apply buffered diagonal movement
            if (rootWindow.pendingXMove !== 0) {
                let targetX = player.x + rootWindow.pendingXMove * 2;
                player.x = Math.max(1, Math.min(gameArea.width - player.width - 1, targetX));
                rootWindow.pendingXMove = 0;
            }
            if (rootWindow.pendingYMove !== 0) {
                let targetY = player.y + rootWindow.pendingYMove;
                player.y = Math.max(1, Math.min(gameArea.height - player.height - 1, targetY));
                rootWindow.pendingYMove = 0;
            }

            rootWindow.tickCounter++;

            // Update level
            let targetLevel = Math.floor(rootWindow.score / 300) + 1;
            if (targetLevel > rootWindow.level) {
                rootWindow.level = targetLevel;
            }

            // --- Power-up Timers ---
            if (rootWindow.firePowerTicks > 0) {
                rootWindow.firePowerTicks--;
                if (rootWindow.firePowerTicks === 0) {
                    rootWindow.fireMode = "single";
                }
            }
            if (rootWindow.shieldTicks > 0) {
                rootWindow.shieldTicks--;
                rootWindow.shieldActive = true;
                if (rootWindow.shieldTicks === 0) {
                    rootWindow.shieldActive = false;
                }
            }

            // --- Scrolling Starfield ---
            // Spawn new stars
            if (rootWindow.tickCounter % 3 === 0 && starsModel.count < 30) {
                starsModel.append({
                    "xPos": Math.floor(Math.random() * (gameArea.width - 2)) + 1,
                    "yPos": 1,
                    "speed": Math.floor(Math.random() * 3) + 1
                });
            }
            // Move stars
            for (let i = starsModel.count - 1; i >= 0; i--) {
                let star = starsModel.get(i);
                let move = false;
                if (star.speed === 3) move = true; // Fast moves every tick
                else if (star.speed === 2 && rootWindow.tickCounter % 2 === 0) move = true; // Med moves every 2 ticks
                else if (star.speed === 1 && rootWindow.tickCounter % 3 === 0) move = true; // Slow moves every 3 ticks

                if (move) {
                    let ny = star.yPos + 1;
                    if (ny < gameArea.height - 1) {
                        starsModel.setProperty(i, "yPos", ny);
                    } else {
                        starsModel.remove(i);
                    }
                }
            }

            // --- Move Bullets ---
            for (let i = bulletsModel.count - 1; i >= 0; i--) {
                let by = bulletsModel.get(i).yPos;
                if (by > 1) {
                    bulletsModel.setProperty(i, "yPos", by - 1);
                } else {
                    bulletsModel.remove(i);
                }
            }

            // --- Move Power-ups ---
            if (rootWindow.tickCounter % 3 === 0) {
                for (let i = powerupsModel.count - 1; i >= 0; i--) {
                    let px = powerupsModel.get(i).xPos;
                    let py = powerupsModel.get(i).yPos;
                    let pType = powerupsModel.get(i).type;

                    let ny = py + 1;
                    if (ny < gameArea.height - 1) {
                        powerupsModel.setProperty(i, "yPos", ny);

                        // Check collision with player
                        if (px >= player.x && px < player.x + player.width &&
                            ny === player.y) {
                            // Apply power-up
                            if (pType === "heal") {
                                rootWindow.health = Math.min(rootWindow.maxHealth, rootWindow.health + 2);
                            } else if (pType === "shield") {
                                rootWindow.shieldTicks = 150; // 7.5 seconds
                                rootWindow.shieldActive = true;
                            } else if (pType === "fire") {
                                rootWindow.fireMode = (rootWindow.fireMode === "single") ? "double" : "triple";
                                rootWindow.firePowerTicks = 200; // 10 seconds
                            }
                            // Spawn flash explosion
                            spawnExplosion(px, ny);
                            powerupsModel.remove(i);
                        }
                    } else {
                        powerupsModel.remove(i);
                    }
                }
            }

            // --- Move Explosions ---
            if (rootWindow.tickCounter % 2 === 0) {
                for (let i = explosionsModel.count - 1; i >= 0; i--) {
                    let frame = explosionsModel.get(i).frame;
                    if (frame < 3) {
                        explosionsModel.setProperty(i, "frame", frame + 1);
                    } else {
                        explosionsModel.remove(i);
                    }
                }
            }

            // --- Spawn Enemies ---
            // Spawn speed scales up with level
            let spawnChance = 0.05 + (rootWindow.level * 0.02);
            if (Math.random() < Math.min(0.25, spawnChance)) {
                let eType = 0; // Basic
                let eHp = 1;
                let eColor = "red";
                let rand = Math.random();

                if (rand < 0.2) {
                    eType = 1; // Heavy Invader
                    eHp = 3;
                    eColor = "magenta";
                } else if (rand < 0.45) {
                    eType = 2; // Zigzag Scout
                    eHp = 1;
                    eColor = "yellow";
                }

                enemiesModel.append({
                    "xPos": Math.floor(Math.random() * (gameArea.width - 4)) + 2,
                    "yPos": 1,
                    "type": eType,
                    "hp": eHp,
                    "enemyColor": eColor,
                    "step": 0
                });
            }

            // --- Move Enemies & Collision with Player ---
            for (let i = enemiesModel.count - 1; i >= 0; i--) {
                let enemy = enemiesModel.get(i);
                let moveTick = false;
                if (enemy.type === 0 && rootWindow.tickCounter % 2 === 0) moveTick = true;
                else if (enemy.type === 1 && rootWindow.tickCounter % 4 === 0) moveTick = true;
                else if (enemy.type === 2 && rootWindow.tickCounter % 2 === 0) moveTick = true;

                if (moveTick) {
                    let ey = enemy.yPos + 1;
                    let ex = enemy.xPos;

                    if (enemy.type === 2) {
                        // Zigzag left/right
                        let step = (enemy.step + 1) % 4;
                        if (step === 0 || step === 1) ex += 1;
                        else ex -= 1;
                        // clamp ex inside game area
                        ex = Math.max(1, Math.min(gameArea.width - 2, ex));
                        enemiesModel.setProperty(i, "step", step);
                    }

                    enemiesModel.setProperty(i, "yPos", ey);
                    enemiesModel.setProperty(i, "xPos", ex);

                    // Check player collision
                    let pxMin = player.x;
                    let pxMax = player.x + player.width - 1;
                    let collided = false;

                    if (enemy.type === 1) { // 3-wide heavy enemy
                        collided = (ex + 2 >= pxMin && ex <= pxMax && ey === player.y);
                    } else { // 1-wide normal
                        collided = (ex >= pxMin && ex <= pxMax && ey === player.y);
                    }

                    if (collided) {
                        spawnExplosion(ex, ey);
                        enemiesModel.remove(i);

                        if (!rootWindow.shieldActive) {
                            rootWindow.health--;
                            if (rootWindow.health <= 0) {
                                rootWindow.lives--;
                                rootWindow.health = rootWindow.maxHealth;
                                // Temp respawn invulnerability
                                rootWindow.shieldActive = true;
                                rootWindow.shieldTicks = 40; // 2 seconds

                                if (rootWindow.lives < 0) {
                                    rootWindow.gameOver = true;
                                    if (rootWindow.score > rootWindow.highScore) {
                                        rootWindow.highScore = rootWindow.score;
                                    }
                                }
                            }
                        }
                        continue;
                    }

                    if (ey >= gameArea.height - 1) {
                        enemiesModel.remove(i);
                        rootWindow.score = Math.max(0, rootWindow.score - 5);
                    }
                }
            }

            // --- Collision: Bullets vs Enemies ---
            for (let i = bulletsModel.count - 1; i >= 0; i--) {
                let bx = bulletsModel.get(i).xPos;
                let by = bulletsModel.get(i).yPos;
                let bulletHit = false;

                for (let j = enemiesModel.count - 1; j >= 0; j--) {
                    let ex = enemiesModel.get(j).xPos;
                    let ey = enemiesModel.get(j).yPos;
                    let et = enemiesModel.get(j).type;
                    let eh = enemiesModel.get(j).hp;

                    // Match overlaps based on width of enemy
                    let hit = false;
                    if (et === 1) { // Heavy enemy (3 wide)
                        hit = (bx >= ex && bx <= ex + 2 && by === ey);
                    } else {
                        hit = (bx === ex && by === ey);
                    }

                    if (hit) {
                        bulletHit = true;
                        eh--;
                        if (eh <= 0) {
                            enemiesModel.remove(j);
                            spawnExplosion(ex, ey);
                            rootWindow.score += (et === 1) ? 30 : ((et === 2) ? 20 : 10);

                            // Chance to spawn power-up
                            if (Math.random() < 0.12) {
                                spawnPowerUp(ex, ey);
                            }
                        } else {
                            enemiesModel.setProperty(j, "hp", eh);
                        }
                        break;
                    }
                }

                if (bulletHit) {
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

        if (event.key === Qt.Key_Left || event.key === Qt.Key_A || event.key === Qt.Key_a) {
            rootWindow.pendingXMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_D || event.key === Qt.Key_d) {
            rootWindow.pendingXMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_W || event.key === Qt.Key_w) {
            rootWindow.pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_S || event.key === Qt.Key_s) {
            rootWindow.pendingYMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_7 || event.key === Qt.Key_U || event.key === Qt.Key_u) {
            rootWindow.pendingXMove = -1
            rootWindow.pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_9 || event.key === Qt.Key_O || event.key === Qt.Key_o) {
            rootWindow.pendingXMove = 1
            rootWindow.pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_1 || event.key === Qt.Key_M || event.key === Qt.Key_m) {
            rootWindow.pendingXMove = -1
            rootWindow.pendingYMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_3 || event.key === Qt.Key_N || event.key === Qt.Key_n) {
            rootWindow.pendingXMove = 1
            rootWindow.pendingYMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_Space) {
            rootWindow.shoot()
            event.accepted = true
        }
    }

    Component.onCompleted: {
        resetGame()
    }

    ListModel { id: starsModel }
    ListModel { id: bulletsModel }
    ListModel { id: enemiesModel }
    ListModel { id: explosionsModel }
    ListModel { id: powerupsModel }

    Rectangle {
        id: gameArea
        anchors.fill: parent
        color: "black"
        border.style: "double"
        border.color: "gray"

        // HUD panel at the top
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            anchors.topMargin: 1
            height: 2
            color: "#111111"
            border.style: "single"
            border.color: "cyan"

            Row {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 2

                Text {
                    text: qsTr(" LIVES: ") + rootWindow.lives + qsTr(" | SHIELD: ") + 
                          (rootWindow.shieldActive ? "[█████]" : "[░░░░░]") + qsTr(" | HP: ") + 
                          ("██████████".substring(0, rootWindow.health) + "░░░░░░░░░░".substring(0, rootWindow.maxHealth - rootWindow.health))
                    color: rootWindow.shieldActive ? "green" : (rootWindow.health <= 1 ? "red" : "yellow")
                    bold: true
                }

                Text {
                    text: qsTr(" | LVL: ") + rootWindow.level + 
                          qsTr(" | HI-SCORE: ") + rootWindow.highScore +
                          qsTr(" | SCORE: ") + rootWindow.score
                    color: "cyan"
                    bold: true
                }

                Text {
                    text: rootWindow.fireMode !== "single" ? qsTr(" | PWR UP: ") + rootWindow.fireMode.toUpperCase() : ""
                    color: "orange"
                    bold: true
                }
            }
        }

        // Render Starfield Background
        Repeater {
            model: starsModel
            delegate: Star {
                x: modelData.xPos
                y: modelData.yPos
                speed: modelData.speed
            }
        }

        // Render Power-ups
        Repeater {
            model: powerupsModel
            delegate: PowerUp {
                x: modelData.xPos
                y: modelData.yPos
                type: modelData.type
            }
        }

        // Render Bullets
        Repeater {
            model: bulletsModel
            delegate: Bullet {
                x: modelData.xPos
                y: modelData.yPos
            }
        }

        // Render Enemies
        Repeater {
            model: enemiesModel
            delegate: Enemy {
                x: modelData.xPos
                y: modelData.yPos
                type: modelData.type
                enemyColor: modelData.enemyColor
            }
        }

        // Render Explosions
        Repeater {
            model: explosionsModel
            delegate: Explosion {
                x: modelData.xPos
                y: modelData.yPos
                frame: modelData.frame
            }
        }

        // Render Player Ship
        Text {
            id: player
            text: rootWindow.shieldActive ? "<A>" : "/A\\"
            color: rootWindow.shieldActive ? "green" : "white"
            width: 3
            height: 1
        }

        // Game Over Panel overlay
        Rectangle {
            visible: rootWindow.gameOver
            anchors.centerIn: parent
            width: 34
            height: 8
            color: "#221111"
            border.style: "double"
            border.color: "red"

            Column {
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: "GAME OVER"
                    color: "red"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }
                Text {
                    text: qsTr("Your Score: ") + rootWindow.score
                    color: "yellow"
                    horizontalAlignment: Qt.AlignHCenter
                }
                Text {
                    text: "Press 'R' to Restart"
                    color: "white"
                    horizontalAlignment: Qt.AlignHCenter
                }
                Text {
                    text: "Press 'Q' to Quit"
                    color: "gray"
                    horizontalAlignment: Qt.AlignHCenter
                }
            }
        }
    }
}
