import QtQml 2.15
import Qtui
import Qtui.Controls

Terminal {
    id: rootWindow

    fillTerminal: true
    color: "black"
    useAlternateScreen: true

    // Game states
    property int score: 0
    property int highScore: 0
    property int level: 1
    property bool gameOver: false
    property bool isPaused: false

    // Coordinates of food
    property int foodX: 0
    property int foodY: 0

    // Snake direction state
    property string direction: "right"
    property string nextDirection: "right"

    // Timer interval calculation
    readonly property int speedInterval: Math.max(50, 180 - level * 15)

    // Grid details (aspect-ratio corrected: 2 columns = 1 row)
    // Dynamically computed from gameField dimensions
    readonly property int gridWidth: Math.max(1, Math.floor((gameField.width - 2) / 2))
    readonly property int gridHeight: Math.max(1, gameField.height - 2)

    // Reset game state
    function resetGame() {
        score = 0
        level = 1
        gameOver = false
        isPaused = false
        direction = "right"
        nextDirection = "right"

        snakeModel.clear()

        // Find center of current grid size
        var cx = Math.floor(gridWidth / 2)
        var cy = Math.floor(gridHeight / 2)

        // Initialize 3-segment snake starting in the middle moving right
        snakeModel.append({ "x": cx, "y": cy })
        snakeModel.append({ "x": cx - 1, "y": cy })
        snakeModel.append({ "x": cx - 2, "y": cy })

        spawnFood()
        gameTimer.restart()
    }

    // Spawn food in a random cell not occupied by the snake
    function spawnFood() {
        if (gridWidth <= 0 || gridHeight <= 0) return
        var valid = false
        var rx = 0
        var ry = 0
        var attempts = 0
        while (!valid && attempts < 100) {
            rx = Math.floor(Math.random() * gridWidth)
            ry = Math.floor(Math.random() * gridHeight)
            attempts++

            // Check if coordinates overlap with snake body
            valid = true
            for (var i = 0; i < snakeModel.count; i++) {
                var segment = snakeModel.get(i)
                if (segment.x === rx && segment.y === ry) {
                    valid = false
                    break
                }
            }
        }
        foodX = rx
        foodY = ry
    }

    // Single step of the snake
    function step() {
        if (gameOver || isPaused || gridWidth <= 0 || gridHeight <= 0) return

        // Update direction to the buffered nextDirection
        direction = nextDirection

        var headX = snakeModel.get(0).x
        var headY = snakeModel.get(0).y

        var nextX = headX
        var nextY = headY

        if (direction === "right") nextX++
        else if (direction === "left") nextX--
        else if (direction === "up") nextY--
        else if (direction === "down") nextY++

        // Edge wrapping logic
        nextX = (nextX + gridWidth) % gridWidth
        nextY = (nextY + gridHeight) % gridHeight

        // Collision Check: self-collision
        for (var i = 0; i < snakeModel.count; i++) {
            var segment = snakeModel.get(i)
            if (segment.x === nextX && segment.y === nextY) {
                gameOver = true
                if (score > highScore) {
                    highScore = score
                }
                return
            }
        }

        // Food Eaten Check
        if (nextX === foodX && nextY === foodY) {
            score += 10

            // Grow: append a copy of the tail segment at the end
            var tailIdx = snakeModel.count - 1
            var tail = snakeModel.get(tailIdx)
            snakeModel.append({ "x": tail.x, "y": tail.y })

            // Shift all segments forward (in-place modification preserves delegates for smooth animation)
            for (var i = snakeModel.count - 2; i > 0; i--) {
                snakeModel.setProperty(i, "x", snakeModel.get(i - 1).x)
                snakeModel.setProperty(i, "y", snakeModel.get(i - 1).y)
            }
            snakeModel.setProperty(0, "x", nextX)
            snakeModel.setProperty(0, "y", nextY)

            // Level up every 50 points
            var newLevel = Math.floor(score / 50) + 1
            if (newLevel !== level) {
                level = newLevel
            }

            // Trigger floating score popup animation
            scorePopup.baseX = (foodX * 2) + 1
            scorePopup.baseY = foodY + 1
            popupAnim.stop()
            popupAnim.start()

            spawnFood()
        } else {
            // Normal move: Shift all segments forward in-place
            for (var i = snakeModel.count - 1; i > 0; i--) {
                snakeModel.setProperty(i, "x", snakeModel.get(i - 1).x)
                snakeModel.setProperty(i, "y", snakeModel.get(i - 1).y)
            }
            snakeModel.setProperty(0, "x", nextX)
            snakeModel.setProperty(0, "y", nextY)
        }
    }

    // Game loop timer
    Timer {
        id: gameTimer
        interval: rootWindow.speedInterval
        running: true
        repeat: true
        onTriggered: {
            rootWindow.step()
        }
    }

    // Keyboard events
    Keys.onPressed: (event) => {
        // Crucial Q/Escape handler for Qtui
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }

        if (gameOver) {
            if (event.key === Qt.Key_R || event.key === Qt.Key_r) {
                resetGame()
                event.accepted = true
            }
            return
        }

        // Handle game pauses
        if (event.key === Qt.Key_Space) {
            isPaused = !isPaused
            event.accepted = true
            return
        }

        if (isPaused) return

        // Buffer direction changes to prevent 180 degree turns
        if ((event.key === Qt.Key_Left || event.key === Qt.Key_A || event.key === Qt.Key_a) && direction !== "right") {
            nextDirection = "left"
            event.accepted = true
        } else if ((event.key === Qt.Key_Right || event.key === Qt.Key_D || event.key === Qt.Key_d) && direction !== "left") {
            nextDirection = "right"
            event.accepted = true
        } else if ((event.key === Qt.Key_Up || event.key === Qt.Key_W || event.key === Qt.Key_w) && direction !== "down") {
            nextDirection = "up"
            event.accepted = true
        } else if ((event.key === Qt.Key_Down || event.key === Qt.Key_S || event.key === Qt.Key_s) && direction !== "up") {
            nextDirection = "down"
            event.accepted = true
        }
    }

    Component.onCompleted: {
        resetGame()
    }

    // Monitor resize safety: respawn food if size changes drastically
    onGridWidthChanged: {
        if (foodX >= gridWidth) spawnFood()
    }
    onGridHeightChanged: {
        if (foodY >= gridHeight) spawnFood()
    }

    // Data models
    ListModel {
        id: snakeModel
    }

    // UI Layout
    Column {
        anchors.fill: parent
        spacing: 0

        // Minimalist Status header
        Rectangle {
            width: parent.width
            height: 1
            color: "#111b15"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    text: "🐍 TUI Snake"
                    color: "lightgreen"
                    bold: true
                }

                Text { text: "|"; color: "gray" }

                Text {
                    text: "Score: " + rootWindow.score
                    color: "yellow"
                    bold: true
                }

                Text {
                    text: "High: " + rootWindow.highScore
                    color: "yellow"
                }

                Text { text: "|"; color: "gray" }

                Text {
                    text: "Level: " + rootWindow.level
                    color: "cyan"
                }

                Text {
                    text: "Speed: " + rootWindow.speedInterval + "ms"
                    color: "magenta"
                }

                Text { text: "|"; color: "gray" }

                Spinner {
                    style: Spinner.Style.Dots
                    color: rootWindow.gameOver ? "red" : (rootWindow.isPaused ? "yellow" : "green")
                }

                Text {
                    text: rootWindow.gameOver ? "GAME OVER" : (rootWindow.isPaused ? "PAUSED" : "RUNNING")
                    color: rootWindow.gameOver ? "red" : (rootWindow.isPaused ? "yellow" : "green")
                    bold: true
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                text: "Move: WASD/Arrows | Pause: Space | Reset: R | Quit: Q"
                color: "gray"
            }
        }

        // Full screen Game Field
        Rectangle {
            id: gameField
            width: parent.width
            height: parent.height - 1
            border.style: "single"
            border.color: "green"
            color: "black"

            // Render food (Pulsing color using ColorAnimation)
            Text {
                id: foodItem
                text: "★ "
                color: "red"
                bold: true
                x: (rootWindow.foodX * 2) + 1
                y: rootWindow.foodY + 1
                width: 2
                height: 1
                visible: rootWindow.gridWidth > 0 && rootWindow.gridHeight > 0

                ColorAnimation {
                    id: foodColorAnim
                    target: foodItem
                    property: "color"
                    running: true
                    duration: 300
                    from: "red"
                    to: "yellow"
                    onFinished: {
                        if (to === "yellow") {
                            from = "yellow";
                            to = "magenta";
                        } else if (to === "magenta") {
                            from = "magenta";
                            to = "red";
                        } else {
                            from = "red";
                            to = "yellow";
                        }
                        stop();
                        start();
                    }
                }

            }

            // Render Snake segments with smooth slide animations
            Repeater {
                model: snakeModel
                delegate: Text {
                    id: segmentText

                    readonly property int safeX: (model.x + rootWindow.gridWidth) % Math.max(1, rootWindow.gridWidth)
                    readonly property int safeY: (model.y + rootWindow.gridHeight) % Math.max(1, rootWindow.gridHeight)

                    property int targetX: (safeX * 2) + 1
                    property int targetY: safeY + 1

                    text: index === 0 ? "██" : "▓▓"
                    color: index === 0 ? "lightgreen" : "green"
                    bold: index === 0

                    x: targetX
                    y: targetY
                    width: 2
                    height: 1

                    // Smooth horizontal glide animation (duration is 0 on wrap jumps to teleport instantly)
                    Behavior on x {
                        NumberAnimation {
                            duration: Math.abs(segmentText.x - segmentText.targetX) <= 2 ? Math.max(30, rootWindow.speedInterval - 15) : 0
                        }
                    }

                    // Smooth vertical glide animation (duration is 0 on wrap jumps to teleport instantly)
                    Behavior on y {
                        NumberAnimation {
                            duration: Math.abs(segmentText.y - segmentText.targetY) <= 1 ? Math.max(30, rootWindow.speedInterval - 15) : 0
                        }
                    }

                }
            }

            // Floating +10 Score popup (Using NumberAnimation on y offset)
            Text {
                id: scorePopup
                text: "+10"
                color: "yellow"
                bold: true
                visible: popupAnim.running

                property int baseX: 0
                property int baseY: 0
                property real offset: 0.0

                x: baseX
                y: baseY - Math.round(offset)
                width: 3
                height: 1

                PropertyAnimation {
                    id: popupAnim
                    target: scorePopup
                    property: "offset"
                    running: false
                    from: 0.0
                    to: 3.0
                    duration: 500
                    easing: Easing.OutQuad
                }


            }

            // Paused Overlay
            Rectangle {
                visible: rootWindow.isPaused && !rootWindow.gameOver
                anchors.fill: parent
                anchors.margins: 1
                color: "black"

                Column {
                    anchors.centerIn: parent
                    spacing: 1

                    Text {
                        text: "P A U S E D"
                        color: "yellow"
                        bold: true
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    Text {
                        text: "Press SPACE to Resume"
                        color: "white"
                        horizontalAlignment: Qt.AlignHCenter
                    }
                }
            }

            // Game Over Overlay
            Rectangle {
                visible: rootWindow.gameOver
                anchors.centerIn: parent
                width: 32
                height: 8
                color: "#2a0808"
                border.style: "single"
                border.color: "red"

                Column {
                    anchors.centerIn: parent
                    spacing: 1

                    Text {
                        text: "G A M E   O V E R"
                        color: "red"
                        bold: true
                        horizontalAlignment: Qt.AlignHCenter
                        width: parent.width
                    }
                    Text {
                        text: "Final Score: " + rootWindow.score
                        color: "yellow"
                        horizontalAlignment: Qt.AlignHCenter
                        width: parent.width
                    }
                    Text {
                        text: "Press R to Restart"
                        color: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        width: parent.width
                    }
                    Text {
                        text: "Press Q to Quit"
                        color: "gray"
                        horizontalAlignment: Qt.AlignHCenter
                        width: parent.width
                    }
                }
            }
        }
    }
}
