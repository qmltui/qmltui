import QtQml
import QtQml.Models
import Qamui

Terminal {
    id: rootWindow
    color: "#02050b"

    property int score: 0
    property int highScore: 0
    property int lives: 3
    property int health: 5
    readonly property int maxHealth: 5
    property int level: 1
    property bool gameOver: false
    property bool isPaused: false
    property bool initialized: false
    property bool resizing: false
    property int tickCounter: 0
    property int pendingXMove: 0
    property int pendingYMove: 0
    property int lastShootTick: -10
    property int damageFlashTicks: 0

    property string fireMode: "single"
    property int firePowerTicks: 0
    property int shieldTicks: 0
    readonly property bool shieldActive: shieldTicks > 0

    readonly property int tickInterval: 60
    readonly property int playLeft: 1
    readonly property int playRight: gameArea.width - 2
    readonly property int playTop: 4
    readonly property int playBottom: gameArea.height - 2
    readonly property int playWidth: Math.max(0, playRight - playLeft + 1)
    readonly property int playHeight: Math.max(0, playBottom - playTop + 1)
    readonly property bool playable: gameArea.width >= 30 && gameArea.height >= 12
    readonly property int shieldSeconds: Math.ceil(shieldTicks * tickInterval / 1000)
    readonly property int fireSeconds: Math.ceil(firePowerTicks * tickInterval / 1000)
    readonly property string fireModeLabel: fireMode === "triple" ? qsTr("TRIPLE")
                                                    : (fireMode === "double" ? qsTr("DOUBLE") : qsTr("SINGLE"))

    function randomInt(minimum, maximum) {
        if (maximum <= minimum)
            return minimum
        return minimum + Math.floor(Math.random() * (maximum - minimum + 1))
    }

    function rangesOverlap(firstStart, firstWidth, secondStart, secondWidth) {
        return firstStart <= secondStart + secondWidth - 1
                && secondStart <= firstStart + firstWidth - 1
    }

    function enemyWidth(enemyType) {
        return enemyType === 0 ? 1 : 3
    }

    function centerPlayer() {
        if (!playable)
            return
        let maximumX = playRight - player.width + 1
        player.x = Math.floor((playLeft + maximumX) / 2)
        player.y = playBottom
    }

    function seedStars() {
        if (!playable)
            return
        while (starsModel.count < 24) {
            starsModel.append({
                "xPos": randomInt(playLeft, playRight),
                "yPos": randomInt(playTop, playBottom),
                "speed": randomInt(1, 3)
            })
        }
    }

    function resetGame() {
        score = 0
        lives = 3
        health = maxHealth
        level = 1
        gameOver = false
        isPaused = false
        tickCounter = 0
        pendingXMove = 0
        pendingYMove = 0
        lastShootTick = -10
        damageFlashTicks = 0
        fireMode = "single"
        firePowerTicks = 0
        shieldTicks = 0

        starsModel.clear()
        bulletsModel.clear()
        enemiesModel.clear()
        explosionsModel.clear()
        powerupsModel.clear()

        if (!playable)
            return
        centerPlayer()
        seedStars()
    }

    function finishGame() {
        lives = 0
        health = 0
        gameOver = true
        highScore = Math.max(highScore, score)
        pendingXMove = 0
        pendingYMove = 0
    }

    function shoot() {
        if (gameOver || isPaused || !playable || tickCounter - lastShootTick < 3)
            return
        if (bulletsModel.count >= 40)
            return

        lastShootTick = tickCounter
        let centerX = player.x + 1
        let bulletY = player.y - 1
        if (bulletY < playTop)
            return

        if (fireMode === "single") {
            bulletsModel.append({ "xPos": centerX, "yPos": bulletY })
        } else if (fireMode === "double") {
            bulletsModel.append({ "xPos": centerX - 1, "yPos": bulletY })
            bulletsModel.append({ "xPos": centerX + 1, "yPos": bulletY })
        } else {
            bulletsModel.append({ "xPos": centerX - 1, "yPos": bulletY })
            bulletsModel.append({ "xPos": centerX, "yPos": Math.max(playTop, bulletY - 1) })
            bulletsModel.append({ "xPos": centerX + 1, "yPos": bulletY })
        }
    }

    function spawnExplosion(explosionX, explosionY) {
        if (explosionsModel.count < 30)
            explosionsModel.append({ "xPos": explosionX, "yPos": explosionY, "frame": 0 })
    }

    function spawnPowerUp(powerX, powerY) {
        if (powerupsModel.count >= 8)
            return

        let roll = Math.random()
        let powerType = roll < 0.35 ? "shield" : (roll < 0.7 ? "fire" : "heal")
        let clampedX = Math.max(playLeft, Math.min(playRight - 2, powerX))
        powerupsModel.append({ "xPos": clampedX, "yPos": powerY, "type": powerType })
    }

    function spawnEnemy() {
        let enemyLimit = Math.min(45, 14 + level * 3)
        if (enemiesModel.count >= enemyLimit)
            return

        let roll = Math.random()
        let enemyType = roll < 0.18 ? 1 : (roll < 0.42 ? 2 : 0)
        let enemyHp = enemyType === 1 ? 3 : 1
        let enemyColor = enemyType === 1 ? "magenta" : (enemyType === 2 ? "yellow" : "red")
        let width = enemyWidth(enemyType)
        enemiesModel.append({
            "xPos": randomInt(playLeft, playRight - width + 1),
            "yPos": playTop,
            "type": enemyType,
            "hp": enemyHp,
            "enemyColor": enemyColor,
            "step": 0
        })
    }

    function applyPendingMovement() {
        if (pendingXMove !== 0) {
            let maximumX = playRight - player.width + 1
            player.x = Math.max(playLeft, Math.min(maximumX, player.x + pendingXMove * 2))
            pendingXMove = 0
        }
        if (pendingYMove !== 0) {
            player.y = Math.max(playTop, Math.min(playBottom, player.y + pendingYMove))
            pendingYMove = 0
        }
    }

    function damagePlayer() {
        if (shieldActive || gameOver)
            return

        --health
        damageFlashTicks = 4
        if (health > 0)
            return

        --lives
        if (lives <= 0) {
            finishGame()
            return
        }

        health = maxHealth
        shieldTicks = 35
        centerPlayer()
        for (let i = enemiesModel.count - 1; i >= 0; --i) {
            if (Math.abs(enemiesModel.get(i).yPos - player.y) <= 2)
                enemiesModel.remove(i)
        }
    }

    function resolvePlayerCollisions() {
        for (let i = enemiesModel.count - 1; i >= 0; --i) {
            let enemy = enemiesModel.get(i)
            if (enemy.yPos === player.y
                    && rangesOverlap(enemy.xPos, enemyWidth(enemy.type), player.x, player.width)) {
                spawnExplosion(enemy.xPos, enemy.yPos)
                enemiesModel.remove(i)
                damagePlayer()
                if (gameOver)
                    return
            }
        }
    }

    function resolveBulletHits() {
        for (let bulletIndex = bulletsModel.count - 1; bulletIndex >= 0; --bulletIndex) {
            let bullet = bulletsModel.get(bulletIndex)
            let bulletHit = false

            for (let enemyIndex = enemiesModel.count - 1; enemyIndex >= 0; --enemyIndex) {
                let enemy = enemiesModel.get(enemyIndex)
                if (bullet.yPos !== enemy.yPos
                        || !rangesOverlap(bullet.xPos, 1, enemy.xPos, enemyWidth(enemy.type)))
                    continue

                bulletHit = true
                let remainingHp = enemy.hp - 1
                if (remainingHp <= 0) {
                    let destroyedX = enemy.xPos
                    let destroyedY = enemy.yPos
                    let destroyedType = enemy.type
                    enemiesModel.remove(enemyIndex)
                    spawnExplosion(destroyedX, destroyedY)
                    score += destroyedType === 1 ? 30 : (destroyedType === 2 ? 20 : 10)
                    highScore = Math.max(highScore, score)
                    if (Math.random() < 0.14)
                        spawnPowerUp(destroyedX, destroyedY)
                } else {
                    enemiesModel.setProperty(enemyIndex, "hp", remainingHp)
                }
                break
            }

            if (bulletHit)
                bulletsModel.remove(bulletIndex)
        }
    }

    function updateStars() {
        if (tickCounter % 3 === 0 && starsModel.count < 30) {
            starsModel.append({
                "xPos": randomInt(playLeft, playRight),
                "yPos": playTop,
                "speed": randomInt(1, 3)
            })
        }

        for (let i = starsModel.count - 1; i >= 0; --i) {
            let star = starsModel.get(i)
            let shouldMove = (star.speed === 3 && tickCounter % 2 === 0)
                    || (star.speed === 2 && tickCounter % 3 === 0)
                    || (star.speed === 1 && tickCounter % 5 === 0)
            if (!shouldMove)
                continue

            if (star.yPos < playBottom)
                starsModel.setProperty(i, "yPos", star.yPos + 1)
            else
                starsModel.remove(i)
        }
    }

    function updateBullets() {
        for (let i = bulletsModel.count - 1; i >= 0; --i) {
            let nextY = bulletsModel.get(i).yPos - 1
            if (nextY >= playTop)
                bulletsModel.setProperty(i, "yPos", nextY)
            else
                bulletsModel.remove(i)
        }
    }

    function updatePowerUps() {
        if (tickCounter % 3 !== 0)
            return

        for (let i = powerupsModel.count - 1; i >= 0; --i) {
            let power = powerupsModel.get(i)
            let nextY = power.yPos + 1
            if (nextY > playBottom) {
                powerupsModel.remove(i)
                continue
            }

            powerupsModel.setProperty(i, "yPos", nextY)
        }
        resolvePowerUpCollisions()
    }

    function resolvePowerUpCollisions() {
        for (let i = powerupsModel.count - 1; i >= 0; --i) {
            let power = powerupsModel.get(i)
            if (power.yPos !== player.y || !rangesOverlap(power.xPos, 3, player.x, player.width))
                continue

            if (power.type === "heal") {
                if (health < maxHealth)
                    health = Math.min(maxHealth, health + 2)
                else
                    score += 10
            } else if (power.type === "shield") {
                shieldTicks = Math.max(shieldTicks, 150)
            } else {
                fireMode = fireMode === "single" ? "double" : "triple"
                firePowerTicks = 200
            }
            spawnExplosion(power.xPos, power.yPos)
            powerupsModel.remove(i)
        }
    }

    function updateExplosions() {
        if (tickCounter % 2 !== 0)
            return
        for (let i = explosionsModel.count - 1; i >= 0; --i) {
            let nextFrame = explosionsModel.get(i).frame + 1
            if (nextFrame <= 3)
                explosionsModel.setProperty(i, "frame", nextFrame)
            else
                explosionsModel.remove(i)
        }
    }

    function updateEnemies() {
        let spawnChance = Math.min(0.18, 0.035 + level * 0.012)
        if (Math.random() < spawnChance)
            spawnEnemy()

        for (let i = enemiesModel.count - 1; i >= 0; --i) {
            let enemy = enemiesModel.get(i)
            let shouldMove = (enemy.type === 0 && tickCounter % 3 === 0)
                    || (enemy.type === 1 && tickCounter % 6 === 0)
                    || (enemy.type === 2 && tickCounter % 3 === 0)
            if (!shouldMove)
                continue

            let nextX = enemy.xPos
            let nextY = enemy.yPos + 1
            if (enemy.type === 2) {
                let nextStep = (enemy.step + 1) % 4
                nextX += nextStep === 0 || nextStep === 1 ? 1 : -1
                nextX = Math.max(playLeft, Math.min(playRight - 2, nextX))
                enemiesModel.setProperty(i, "step", nextStep)
            }

            if (nextY > playBottom) {
                enemiesModel.remove(i)
                score = Math.max(0, score - 5)
                continue
            }

            enemiesModel.setProperty(i, "xPos", nextX)
            enemiesModel.setProperty(i, "yPos", nextY)
        }
    }

    function updatePowerTimers() {
        if (firePowerTicks > 0) {
            --firePowerTicks
            if (firePowerTicks === 0)
                fireMode = "single"
        }
        if (shieldTicks > 0)
            --shieldTicks
        if (damageFlashTicks > 0)
            --damageFlashTicks
    }

    function gameStep() {
        if (gameOver || isPaused || resizing || !playable)
            return

        ++tickCounter
        applyPendingMovement()
        resolvePowerUpCollisions()
        resolvePlayerCollisions()
        if (gameOver)
            return

        updatePowerTimers()
        updateStars()
        updateBullets()
        resolveBulletHits()
        updatePowerUps()
        updateExplosions()
        updateEnemies()
        resolveBulletHits()
        resolvePlayerCollisions()
        level = Math.floor(score / 250) + 1
    }

    function scheduleResize() {
        if (!initialized)
            return
        resizing = true
        resizeTimer.restart()
    }

    function finishResize() {
        resizing = false
        if (!playable)
            return

        let maximumPlayerX = playRight - player.width + 1
        player.x = Math.max(playLeft, Math.min(maximumPlayerX, player.x))
        player.y = Math.max(playTop, Math.min(playBottom, player.y))

        for (let i = starsModel.count - 1; i >= 0; --i) {
            let star = starsModel.get(i)
            if (star.xPos < playLeft || star.xPos > playRight
                    || star.yPos < playTop || star.yPos > playBottom)
                starsModel.remove(i)
        }
        for (let i = bulletsModel.count - 1; i >= 0; --i) {
            let bullet = bulletsModel.get(i)
            if (bullet.xPos < playLeft || bullet.xPos > playRight
                    || bullet.yPos < playTop || bullet.yPos > playBottom)
                bulletsModel.remove(i)
        }
        for (let i = enemiesModel.count - 1; i >= 0; --i) {
            let enemy = enemiesModel.get(i)
            if (enemy.xPos < playLeft || enemy.xPos + enemyWidth(enemy.type) - 1 > playRight
                    || enemy.yPos < playTop || enemy.yPos > playBottom)
                enemiesModel.remove(i)
        }
        for (let i = powerupsModel.count - 1; i >= 0; --i) {
            let power = powerupsModel.get(i)
            if (power.xPos < playLeft || power.xPos + 2 > playRight
                    || power.yPos < playTop || power.yPos > playBottom)
                powerupsModel.remove(i)
        }
        seedStars()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_R
                || (gameOver && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter))) {
            resetGame()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_P && !gameOver && playable) {
            isPaused = !isPaused
            event.accepted = true
            return
        }

        if (gameOver || isPaused || !playable)
            return

        if (event.key === Qt.Key_Space) {
            shoot()
            event.accepted = true
        } else if (event.key === Qt.Key_Left || event.key === Qt.Key_A) {
            pendingXMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_D) {
            pendingXMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_W) {
            pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_S) {
            pendingYMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_7 || event.key === Qt.Key_U) {
            pendingXMove = -1
            pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_9 || event.key === Qt.Key_O) {
            pendingXMove = 1
            pendingYMove = -1
            event.accepted = true
        } else if (event.key === Qt.Key_1 || event.key === Qt.Key_M) {
            pendingXMove = -1
            pendingYMove = 1
            event.accepted = true
        } else if (event.key === Qt.Key_3 || event.key === Qt.Key_N) {
            pendingXMove = 1
            pendingYMove = 1
            event.accepted = true
        }
    }

    Component.onCompleted: {
        initialized = true
        resetGame()
    }

    onWidthChanged: scheduleResize()
    onHeightChanged: scheduleResize()

    ListModel { id: starsModel }
    ListModel { id: bulletsModel }
    ListModel { id: enemiesModel }
    ListModel { id: explosionsModel }
    ListModel { id: powerupsModel }

    Timer {
        interval: rootWindow.tickInterval
        running: rootWindow.initialized && rootWindow.playable && !rootWindow.gameOver
                 && !rootWindow.isPaused && !rootWindow.resizing
        repeat: true
        onTriggered: rootWindow.gameStep()
    }

    Timer {
        id: resizeTimer
        interval: 100
        repeat: false
        onTriggered: rootWindow.finishResize()
    }

    Rectangle {
        id: gameArea
        anchors.fill: parent
        color: "#02050b"
        border.style: "double"
        border.color: rootWindow.gameOver ? "#823247" : "#38506e"

        Repeater {
            model: starsModel
            delegate: Star {
                required property var modelData
                x: modelData.xPos
                y: modelData.yPos
                speed: modelData.speed
            }
        }

        Repeater {
            model: powerupsModel
            delegate: PowerUp {
                required property var modelData
                x: modelData.xPos
                y: modelData.yPos
                type: modelData.type
            }
        }

        Repeater {
            model: bulletsModel
            delegate: Bullet {
                required property var modelData
                x: modelData.xPos
                y: modelData.yPos
            }
        }

        Repeater {
            model: enemiesModel
            delegate: Enemy {
                required property var modelData
                x: modelData.xPos
                y: modelData.yPos
                type: modelData.type
                enemyColor: modelData.enemyColor
            }
        }

        Repeater {
            model: explosionsModel
            delegate: Explosion {
                required property var modelData
                x: modelData.xPos
                y: modelData.yPos
                frame: modelData.frame
            }
        }

        Text {
            id: player
            text: rootWindow.shieldActive ? "<A>" : "/A\\"
            color: rootWindow.damageFlashTicks > 0 ? "red"
                   : (rootWindow.shieldActive ? "cyan" : "white")
            width: 3
            height: 1
            bold: rootWindow.shieldActive || rootWindow.damageFlashTicks > 0
        }

        Rectangle {
            id: hud
            x: 1
            y: 1
            width: parent.width - 2
            height: 3
            color: "#07111f"
            border.style: "single"
            border.color: "#2c789e"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.top: parent.top
                text: qsTr("LIVES %1  HP %2").arg(rootWindow.lives)
                        .arg("█████".substring(0, rootWindow.health)
                             + "░░░░░".substring(0, rootWindow.maxHealth - rootWindow.health))
                color: rootWindow.health <= 1 ? "#ff5d73" : "#ffd166"
                bold: true
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.top: parent.top
                text: parent.width < 58
                      ? qsTr("%1 pts").arg(rootWindow.score)
                      : qsTr("SCORE %1  BEST %2  WAVE %3")
                            .arg(rootWindow.score).arg(rootWindow.highScore).arg(rootWindow.level)
                color: "#64d8ff"
                bold: true
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.bottom: parent.bottom
                text: parent.width < 72
                      ? qsTr("MOVE WASD  ·  FIRE SPACE  ·  PAUSE P")
                      : qsTr("MOVE WASD/ARROWS  ·  FIRE SPACE  ·  PAUSE P  ·  RESTART R  ·  QUIT Q")
                color: "#7f96ad"
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.bottom: parent.bottom
                visible: parent.width >= 72
                text: rootWindow.shieldActive
                      ? qsTr("SHIELD %1s").arg(rootWindow.shieldSeconds)
                      : (rootWindow.fireMode !== "single"
                         ? qsTr("%1 FIRE %2s").arg(rootWindow.fireModeLabel).arg(rootWindow.fireSeconds)
                         : (rootWindow.isPaused ? qsTr("PAUSED") : qsTr("READY")))
                color: rootWindow.shieldActive ? "#57e3ff"
                       : (rootWindow.fireMode !== "single" ? "#ffb000" : "#57e389")
                bold: true
            }
        }

        Rectangle {
            id: sizeWarning
            anchors.centerIn: parent
            readonly property bool compact: width < 30 || gameArea.height < 12
            width: Math.max(3, Math.min(parent.width - 2, 42))
            height: compact ? Math.max(1, Math.min(parent.height - 2, 3)) : 7
            visible: !rootWindow.playable
            color: "#17120b"
            border.style: "single"
            border.color: "#ffd166"

            Column {
                anchors.centerIn: parent
                width: parent.width - 2
                spacing: 1
                Text {
                    width: parent.width
                    text: sizeWarning.compact ? qsTr("TOO SMALL") : qsTr("TERMINAL TOO SMALL")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#ffd166"
                    bold: true
                }
                Text {
                    width: parent.width
                    visible: !sizeWarning.compact
                    text: qsTr("Resize to at least 30 × 12")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#d7c7a4"
                }
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 4, 36)
            height: 5
            visible: rootWindow.isPaused && !rootWindow.gameOver && rootWindow.playable
            color: "#111827"
            border.style: "single"
            border.color: "#ffd166"

            Column {
                anchors.centerIn: parent
                width: parent.width - 2
                spacing: 1
                Text {
                    width: parent.width
                    text: qsTr("MISSION PAUSED")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#ffd166"
                    bold: true
                }
                Text {
                    width: parent.width
                    text: qsTr("Press P to continue")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#dce8f5"
                }
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 4, 42)
            height: 9
            visible: rootWindow.gameOver && rootWindow.playable
            color: "#260c16"
            border.style: "double"
            border.color: "#ff5d73"

            Column {
                anchors.centerIn: parent
                width: parent.width - 2
                spacing: 1
                Text {
                    width: parent.width
                    text: qsTr("MISSION FAILED")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#ff5d73"
                    bold: true
                }
                Text {
                    width: parent.width
                    text: qsTr("Score %1  ·  Best %2  ·  Wave %3")
                            .arg(rootWindow.score).arg(rootWindow.highScore).arg(rootWindow.level)
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#ffd166"
                }
                Text {
                    width: parent.width
                    text: qsTr("R / ENTER to redeploy")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#dce8f5"
                }
                Text {
                    width: parent.width
                    text: qsTr("Q / ESC to quit")
                    horizontalAlignment: Qt.AlignHCenter
                    color: "#7f96ad"
                }
            }
        }
    }
}
