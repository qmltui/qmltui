import QtQml
import QtQml.Models
import Qtui
import Qtui.Canvas

Terminal {
    id: rootWindow

    fillTerminal: true
    color: "#030806"
    useAlternateScreen: true

    property int score: 0
    property int highScore: 0
    property int level: 1
    property bool gameOver: false
    property bool won: false
    property bool isPaused: false
    property bool initialized: false
    property bool resizing: false

    property int foodX: 0
    property int foodY: 0
    property bool foodPulse: false

    property string direction: "right"
    property var directionQueue: []
    property int movementTick: 0

    readonly property int cellPixelWidth: 2
    readonly property int cellPixelHeight: 2
    readonly property int gridWidth: Math.floor(gameCanvas.pixelWidth / cellPixelWidth)
    readonly property int gridHeight: Math.floor(gameCanvas.pixelHeight / cellPixelHeight)
    readonly property bool playable: gridWidth >= 6 && gridHeight >= 5
    readonly property int speedInterval: Math.max(48, 105 - ((level - 1) * 6))

    function isSnakeAt(cellX, cellY, limit) {
        let count = limit === undefined ? snakeModel.count : limit
        for (let i = 0; i < count; ++i) {
            let segment = snakeModel.get(i)
            if (segment.cellX === cellX && segment.cellY === cellY)
                return true
        }
        return false
    }

    function paintCell(cellX, cellY, cellColor) {
        let originX = cellX * cellPixelWidth
        let originY = cellY * cellPixelHeight
        for (let pixelY = 0; pixelY < cellPixelHeight; ++pixelY) {
            for (let pixelX = 0; pixelX < cellPixelWidth; ++pixelX)
                gameCanvas.setPixelColor(originX + pixelX, originY + pixelY, cellColor)
        }
    }

    function redrawCanvas() {
        gameCanvas.clear()
        if (!playable || snakeModel.count === 0)
            return

        if (!gameOver) {
            paintCell(foodX, foodY, foodPulse ? "#ffb000" : "#ff4365")
            gameCanvas.setPixelColor(foodX * cellPixelWidth + 1,
                                     foodY * cellPixelHeight, "#b7f34a")
        }

        for (let i = snakeModel.count - 1; i >= 0; --i) {
            let segment = snakeModel.get(i)
            let segmentColor = i === 0 ? "#82ff9f" : (i % 2 === 0 ? "#22b455" : "#2bdc68")
            paintCell(segment.cellX, segment.cellY, segmentColor)
        }

        let head = snakeModel.get(0)
        let eyeX = head.cellX * cellPixelWidth + (direction === "right" ? 1 : 0)
        let eyeY = head.cellY * cellPixelHeight + (direction === "down" ? 1 : 0)
        gameCanvas.setPixelColor(eyeX, eyeY, "#07150c")
    }

    function resetGame() {
        score = 0
        level = 1
        gameOver = false
        won = false
        isPaused = false
        direction = "right"
        directionQueue = []
        movementTick = 0
        foodPulse = false
        snakeModel.clear()

        if (!playable) {
            redrawCanvas()
            return
        }

        let centerX = Math.floor(gridWidth / 2)
        let centerY = Math.floor(gridHeight / 2)
        snakeModel.append({ "cellX": centerX, "cellY": centerY })
        snakeModel.append({ "cellX": centerX - 1, "cellY": centerY })
        snakeModel.append({ "cellX": centerX - 2, "cellY": centerY })
        spawnFood()
        redrawCanvas()
    }

    function spawnFood() {
        if (!playable || snakeModel.count >= gridWidth * gridHeight)
            return

        let cellCount = gridWidth * gridHeight
        let firstCell = Math.floor(Math.random() * cellCount)
        for (let offset = 0; offset < cellCount; ++offset) {
            let candidate = (firstCell + offset) % cellCount
            let candidateX = candidate % gridWidth
            let candidateY = Math.floor(candidate / gridWidth)
            if (!isSnakeAt(candidateX, candidateY)) {
                foodX = candidateX
                foodY = candidateY
                return
            }
        }

        won = true
        gameOver = true
    }

    function finishGame(didWin) {
        won = didWin
        gameOver = true
        highScore = Math.max(highScore, score)
        redrawCanvas()
    }

    function queueDirection(candidate) {
        let queuedDirections = directionQueue.slice()
        let previousDirection = queuedDirections.length > 0
                ? queuedDirections[queuedDirections.length - 1] : direction
        if (candidate === previousDirection || queuedDirections.length >= 2)
            return

        let isReverse = (previousDirection === "left" && candidate === "right")
                || (previousDirection === "right" && candidate === "left")
                || (previousDirection === "up" && candidate === "down")
                || (previousDirection === "down" && candidate === "up")
        if (isReverse)
            return

        queuedDirections.push(candidate)
        directionQueue = queuedDirections
    }

    function step() {
        if (!playable || gameOver || isPaused || resizing || snakeModel.count === 0)
            return

        if (directionQueue.length > 0) {
            let queuedDirections = directionQueue.slice()
            direction = queuedDirections.shift()
            directionQueue = queuedDirections
        }

        let head = snakeModel.get(0)
        let nextX = head.cellX
        let nextY = head.cellY

        if (direction === "right")
            ++nextX
        else if (direction === "left")
            --nextX
        else if (direction === "up")
            --nextY
        else
            ++nextY

        nextX = (nextX + gridWidth) % gridWidth
        nextY = (nextY + gridHeight) % gridHeight

        let ateFood = nextX === foodX && nextY === foodY
        // On a normal move the tail leaves its cell, so entering that cell is legal.
        let collisionCount = snakeModel.count - (ateFood ? 0 : 1)
        if (isSnakeAt(nextX, nextY, collisionCount)) {
            finishGame(false)
            return
        }

        if (ateFood) {
            let tail = snakeModel.get(snakeModel.count - 1)
            snakeModel.append({ "cellX": tail.cellX, "cellY": tail.cellY })
        }

        for (let i = snakeModel.count - 1; i > 0; --i) {
            let previous = snakeModel.get(i - 1)
            snakeModel.setProperty(i, "cellX", previous.cellX)
            snakeModel.setProperty(i, "cellY", previous.cellY)
        }
        snakeModel.setProperty(0, "cellX", nextX)
        snakeModel.setProperty(0, "cellY", nextY)

        ++movementTick
        if (movementTick % 3 === 0)
            foodPulse = !foodPulse

        if (ateFood) {
            score += 10
            highScore = Math.max(highScore, score)
            level = Math.floor(score / 50) + 1
            if (snakeModel.count === gridWidth * gridHeight)
                finishGame(true)
            else
                spawnFood()
        }
        redrawCanvas()
    }

    function scheduleResize() {
        if (!initialized)
            return
        resizing = true
        resizeTimer.restart()
    }

    function finishResize() {
        resizing = false
        if (!playable) {
            redrawCanvas()
            return
        }

        for (let i = 0; i < snakeModel.count; ++i) {
            let segment = snakeModel.get(i)
            if (segment.cellX >= gridWidth || segment.cellY >= gridHeight) {
                resetGame()
                return
            }
        }

        if (snakeModel.count === 0)
            resetGame()
        else if (foodX >= gridWidth || foodY >= gridHeight || isSnakeAt(foodX, foodY))
            spawnFood()
        redrawCanvas()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_R || (gameOver && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter))) {
            resetGame()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Space && !gameOver && playable) {
            isPaused = !isPaused
            event.accepted = true
            return
        }

        if (gameOver || isPaused || !playable)
            return

        if (event.key === Qt.Key_Left || event.key === Qt.Key_A) {
            queueDirection("left")
            event.accepted = true
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_D) {
            queueDirection("right")
            event.accepted = true
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_W) {
            queueDirection("up")
            event.accepted = true
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_S) {
            queueDirection("down")
            event.accepted = true
        }
    }

    Component.onCompleted: {
        initialized = true
        resetGame()
    }

    onGridWidthChanged: scheduleResize()
    onGridHeightChanged: scheduleResize()

    ListModel {
        id: snakeModel
    }

    Timer {
        interval: rootWindow.speedInterval
        running: rootWindow.initialized && rootWindow.playable && !rootWindow.gameOver
                 && !rootWindow.isPaused && !rootWindow.resizing
        repeat: true
        onTriggered: rootWindow.step()
    }

    Timer {
        id: resizeTimer
        interval: 100
        repeat: false
        onTriggered: rootWindow.finishResize()
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header
            width: parent.width
            height: 3
            color: "#0b1711"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.top: parent.top
                text: parent.width < 64 ? qsTr("SNAKE") : qsTr("SNAKE // HALF-BLOCK EDITION")
                color: "#7dff9b"
                bold: true
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.top: parent.top
                text: parent.width < 54
                      ? qsTr("%1 pts").arg(rootWindow.score)
                      : qsTr("SCORE %1  BEST %2  LVL %3")
                            .arg(rootWindow.score).arg(rootWindow.highScore).arg(rootWindow.level)
                color: "#ffd166"
                bold: true
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.bottom: parent.bottom
                text: parent.width < 70
                      ? qsTr("WASD/ARROWS · SPACE · R · Q")
                      : qsTr("MOVE WASD/ARROWS  ·  PAUSE SPACE  ·  RESTART R  ·  QUIT Q")
                color: "#789488"
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.bottom: parent.bottom
                visible: parent.width >= 70
                text: rootWindow.gameOver
                      ? (rootWindow.won ? qsTr("BOARD CLEARED") : qsTr("GAME OVER"))
                      : (rootWindow.isPaused ? qsTr("PAUSED") : qsTr("RUNNING · %1ms").arg(rootWindow.speedInterval))
                color: rootWindow.gameOver ? "#ff5d73" : (rootWindow.isPaused ? "#ffd166" : "#57e389")
                bold: true
            }
        }

        Rectangle {
            id: gameField
            width: parent.width
            height: parent.height - header.height
            color: "#030806"
            border.style: "single"
            border.color: rootWindow.gameOver ? "#8f3045" : "#225c38"

            Canvas {
                id: gameCanvas
                anchors.fill: parent
                anchors.margins: 1
                renderMode: Canvas.HalfBlock
                autoClear: false
                fillColor: "#030806"
            }

            Rectangle {
                id: sizeWarning
                anchors.centerIn: parent
                readonly property bool compact: width < 30 || gameField.height < 9
                width: Math.max(3, Math.min(parent.width - 2, 38))
                height: compact ? Math.max(1, Math.min(parent.height - 2, 3)) : 7
                visible: !rootWindow.playable
                color: "#17120b"
                border.style: "single"
                border.color: "#ffd166"
                Column {
                    anchors.centerIn: parent
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
                        text: qsTr("Resize to at least 14 × 10")
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#d7c7a4"
                    }
                    Text {
                        width: parent.width
                        visible: !sizeWarning.compact
                        text: qsTr("Current grid: %1 × %2").arg(rootWindow.gridWidth).arg(rootWindow.gridHeight)
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#789488"
                    }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 4, 38)
                height: 9
                visible: rootWindow.gameOver && rootWindow.playable
                color: rootWindow.won ? "#092417" : "#240c13"
                border.style: "double"
                border.color: rootWindow.won ? "#57e389" : "#ff5d73"
                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        width: parent.width
                        text: rootWindow.won ? qsTr("BOARD CLEARED!") : qsTr("GAME OVER")
                        horizontalAlignment: Qt.AlignHCenter
                        color: rootWindow.won ? "#57e389" : "#ff5d73"
                        bold: true
                    }
                    Text {
                        width: parent.width
                        text: qsTr("Score %1  ·  Best %2").arg(rootWindow.score).arg(rootWindow.highScore)
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#ffd166"
                    }
                    Text {
                        width: parent.width
                        text: qsTr("R / ENTER to play again")
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#e4eee8"
                    }
                    Text {
                        width: parent.width
                        text: qsTr("Q / ESC to quit")
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#789488"
                    }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 28
                height: 5
                visible: rootWindow.isPaused && !rootWindow.gameOver && rootWindow.playable
                color: "#17120b"
                border.style: "single"
                border.color: "#ffd166"
                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        width: parent.width
                        text: qsTr("PAUSED")
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#ffd166"
                        bold: true
                    }
                    Text {
                        width: parent.width
                        text: qsTr("SPACE to continue")
                        horizontalAlignment: Qt.AlignHCenter
                        color: "#e4eee8"
                    }
                }
            }
        }
    }
}
