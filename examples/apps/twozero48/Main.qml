pragma ComponentBehavior: Bound

import QtQml
import Qtui

Terminal {
    id: rootWindow
    color: "#faf8ef"
    focus: true

    property int score: 0
    property int highScore: 0
    property bool gameOver: false
    property bool won: false
    property bool keepPlaying: false

    readonly property bool playable: rootWindow.width >= 42 && rootWindow.height >= 20

    // 1D array representing the 4x4 grid board
    property var board: [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    ]

    Component.onCompleted: {
        initializeGame();
    }

    function initializeGame() {
        board = [
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        ];
        score = 0;
        gameOver = false;
        won = false;
        keepPlaying = false;
        spawnNewTile();
        spawnNewTile();
    }

    function spawnNewTile() {
        let emptyIndices = [];
        for (let i = 0; i < 16; i++) {
            if (board[i] === 0) {
                emptyIndices.push(i);
            }
        }
        if (emptyIndices.length > 0) {
            let randIdx = emptyIndices[Math.floor(Math.random() * emptyIndices.length)];
            let val = Math.random() < 0.9 ? 2 : 4;
            let nextBoard = [...board];
            nextBoard[randIdx] = val;
            board = nextBoard;
        }
    }

    function slideAndMerge(arr, scoreAdder) {
        let nonZero = arr.filter(x => x > 0);
        let merged = [];
        for (let i = 0; i < nonZero.length; i++) {
            if (i < nonZero.length - 1 && nonZero[i] === nonZero[i+1]) {
                let mergedVal = nonZero[i] * 2;
                merged.push(mergedVal);
                scoreAdder.val += mergedVal;
                if (mergedVal === 2048) {
                    rootWindow.won = true;
                }
                i++;
            } else {
                merged.push(nonZero[i]);
            }
        }
        while (merged.length < 4) {
            merged.push(0);
        }
        return merged;
    }

    function move(direction) {
        if (gameOver) return;

        let prevBoard = [...board];
        let currentBoard = [...board];
        let scoreAdder = { val: 0 };
        let changed = false;

        if (direction === "left") {
            for (let r = 0; r < 4; r++) {
                let row = [currentBoard[r*4], currentBoard[r*4+1], currentBoard[r*4+2], currentBoard[r*4+3]];
                let nextRow = slideAndMerge(row, scoreAdder);
                for (let c = 0; c < 4; c++) {
                    currentBoard[r*4 + c] = nextRow[c];
                }
            }
        } else if (direction === "right") {
            for (let r = 0; r < 4; r++) {
                let row = [currentBoard[r*4+3], currentBoard[r*4+2], currentBoard[r*4+1], currentBoard[r*4]];
                let nextRow = slideAndMerge(row, scoreAdder);
                for (let c = 0; c < 4; c++) {
                    currentBoard[r*4 + 3 - c] = nextRow[c];
                }
            }
        } else if (direction === "up") {
            for (let c = 0; c < 4; c++) {
                let col = [currentBoard[c], currentBoard[4+c], currentBoard[8+c], currentBoard[12+c]];
                let nextCol = slideAndMerge(col, scoreAdder);
                for (let r = 0; r < 4; r++) {
                    currentBoard[r*4 + c] = nextCol[r];
                }
            }
        } else if (direction === "down") {
            for (let c = 0; c < 4; c++) {
                let col = [currentBoard[12+c], currentBoard[8+c], currentBoard[4+c], currentBoard[c]];
                let nextCol = slideAndMerge(col, scoreAdder);
                for (let r = 0; r < 4; r++) {
                    currentBoard[(3-r)*4 + c] = nextCol[r];
                }
            }
        }

        // Check if anything changed
        for (let i = 0; i < 16; i++) {
            if (currentBoard[i] !== prevBoard[i]) {
                changed = true;
                break;
            }
        }

        if (changed) {
            board = currentBoard;
            score += scoreAdder.val;
            if (score > highScore) {
                highScore = score;
            }
            spawnNewTile();
            checkGameOver();
        }
    }

    function checkGameOver() {
        // Any empty spaces?
        for (let i = 0; i < 16; i++) {
            if (board[i] === 0) return;
        }
        // Any adjacent identical values?
        for (let r = 0; r < 4; r++) {
            for (let c = 0; c < 4; c++) {
                let idx = r * 4 + c;
                let val = board[idx];
                if (c < 3 && board[idx + 1] === val) return;
                if (r < 3 && board[idx + 4] === val) return;
            }
        }
        gameOver = true;
    }

    // Key handlers
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_R || (gameOver && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter))) {
            initializeGame();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_C && won && !keepPlaying) {
            keepPlaying = true;
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_Left || event.key === Qt.Key_A || event.key === Qt.Key_H) {
            move("left");
            event.accepted = true;
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_D || event.key === Qt.Key_L) {
            move("right");
            event.accepted = true;
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_W || event.key === Qt.Key_K) {
            move("up");
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_S || event.key === Qt.Key_J) {
            move("down");
            event.accepted = true;
        }
    }

    // Authentic 2048 styling mappings
    function getCellBg(val) {
        switch (val) {
            case 0: return "#ccc0b3";
            case 2: return "#eee4da";
            case 4: return "#ede0c8";
            case 8: return "#f2b179";
            case 16: return "#f59563";
            case 32: return "#f67c5f";
            case 64: return "#f65e3b";
            case 128: return "#edcf72";
            case 256: return "#edcc61";
            case 512: return "#edc850";
            case 1024: return "#edc53f";
            case 2048: return "#edc22e";
            default: return "#3c3a32";
        }
    }

    function getCellText(val) {
        if (val === 2 || val === 4) {
            return "#776e65";
        }
        return "#f9f6f2";
    }

    // Visual layout structure
    Column {
        anchors.centerIn: parent
        spacing: 1
        visible: rootWindow.playable

        // Title and Score Header
        Row {
            spacing: 2
            anchors.horizontalCenter: parent.horizontalCenter

            // App Title Banner
            Rectangle {
                width: 17
                height: 3
                border.style: "none"
                color: "#edc22e"

                Text {
                    anchors.centerIn: parent
                    text: "2048 TUI"
                    color: "#ffffff"
                    bold: true
                }
            }

            // Current Score Box
            Rectangle {
                width: 13
                height: 3
                border.style: "none"
                color: "#bbada0"

                Column {
                    anchors.centerIn: parent
                    Text {
                        text: "SCORE"
                        color: "#eee4da"
                        bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: String(rootWindow.score)
                        color: "#ffffff"
                        bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // High Score Box
            Rectangle {
                width: 13
                height: 3
                border.style: "none"
                color: "#bbada0"

                Column {
                    anchors.centerIn: parent
                    Text {
                        text: "BEST"
                        color: "#eee4da"
                        bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: String(rootWindow.highScore)
                        color: "#ffffff"
                        bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }

        // Subtitle / Instruction Text
        Text {
            text: "Join the numbers to get 2048! [Arrows/WASD/HJKL] Move • [R] Reset • [Q] Quit"
            color: "#776e65"
            bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Grid Container (Authentic 2048 board)
        Rectangle {
            id: gridBorder
            width: 39
            height: 17
            border.style: "none"
            color: "#bbada0"
            anchors.horizontalCenter: parent.horizontalCenter

            // 4x4 Grid layout
            Column {
                anchors.centerIn: parent
                spacing: 1

                Repeater {
                    model: 4
                    Row {
                        id: boardRow
                        required property int index
                        readonly property int rIdx: boardRow.index
                        spacing: 1

                        Repeater {
                            model: 4
                            Rectangle {
                                id: boardCell
                                required property int index
                                readonly property int cIdx: boardCell.index
                                readonly property int val: rootWindow.board[boardRow.rIdx * 4 + boardCell.cIdx]

                                width: 8
                                height: 3
                                color: rootWindow.getCellBg(boardCell.val)
                                border.style: "none"

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: boardCell.val > 0 ? String(boardCell.val) : ""
                                    color: rootWindow.getCellText(boardCell.val)
                                    bold: true
                                    horizontalAlignment: Qt.AlignHCenter
                                    verticalAlignment: Qt.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }

            // Banner Overlays for Win / Loss States
            Rectangle {
                anchors.centerIn: parent
                width: 35
                height: 9
                visible: rootWindow.gameOver || (rootWindow.won && !rootWindow.keepPlaying)
                color: rootWindow.won ? "#edc22e" : "#eee4da"
                border.style: "single"
                border.color: rootWindow.won ? "#f59e0b" : "#8f7a66"

                Column {
                    anchors.centerIn: parent
                    spacing: 1

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: rootWindow.gameOver ? "Game Over!" : "You Win!"
                        color: rootWindow.won ? "#ffffff" : "#776e65"
                        bold: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Final Score: " + rootWindow.score
                        color: rootWindow.won ? "#ffffff" : "#776e65"
                        bold: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: rootWindow.gameOver ? "Press [R] to try again" : "Press [C] to keep going  •  [R] reset"
                        color: rootWindow.won ? "#f9f6f2" : "#8f7a66"
                    }
                }
            }
        }
    }

    // Size Warning
    Rectangle {
        anchors.centerIn: parent
        width: 32
        height: 5
        visible: !rootWindow.playable
        border.style: "single"
        border.color: "#bbada0"
        color: "#eee4da"

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "TERMINAL TOO SMALL"
                color: "#776e65"
                bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Resize to at least 42 × 20"
                color: "#8f7a66"
            }
        }
    }
}
