pragma ComponentBehavior: Bound

import QtQml 2.15
import Qtui


Terminal {
    id: rootWindow
    fillTerminal: true
    color: "#faf8ef"
    useAlternateScreen: true

    property int score: 0
    property int highScore: 0
    property bool gameOver: false
    property bool won: false

    property real animatedScore: score
    Behavior on animatedScore {
        NumberAnimation {
            duration: 300
        }
    }

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
        if (event.key === Qt.Key_R) {
            initializeGame();
            event.accepted = true;
            return;
        }
        
        if (event.key === Qt.Key_Left || event.key === Qt.Key_A) {
            move("left");
            event.accepted = true;
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_D) {
            move("right");
            event.accepted = true;
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_W) {
            move("up");
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_S) {
            move("down");
            event.accepted = true;
        }
    }

    // Cell Styling Mappings
    function getCellBgColor(val) {
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

    function getCellTextColor(val) {
        if (val === 2 || val === 4) {
            return "#776e65";
        }
        return "#f9f6f2";
    }

    function getCellBorderStyle(val) {
        return "none";
    }

    function getCellBorderColor(val) {
        return "transparent";
    }

    // Visual layout structure
    Column {
        anchors.centerIn: parent
        spacing: 2

        // Title and Highscore Bar
        Row {
            spacing: 4
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

            // Current Score
            Rectangle {
                width: 12
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
                        text: String(Math.round(rootWindow.animatedScore))
                        color: "#ffffff"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // High Score
            Rectangle {
                width: 12
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
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }

        // Subtitle / Instruction Text
        Text {
            text: "Use [Arrow keys / WASD] to Slide | [R] Restart | [Q / Esc] Quit"
            color: "#776e65"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Space divider
        Text { text: ""; height: 1 }

        // Grid Container
        Rectangle {
            id: gridBorder
            width: 46
            height: 17
            border.style: "single"
            border.color: "#bbada0"
            color: "#bbada0"
            padding: 1
            anchors.horizontalCenter: parent.horizontalCenter

            // 4x4 Grid layout
            Column {
                anchors.centerIn: parent
                spacing: 1

                Repeater {
                    model: 4
                    delegate: Row {
                        id: boardRow
                        required property int index
                        readonly property int rIdx: boardRow.index
                        spacing: 2

                        Repeater {
                            model: 4
                            delegate: Rectangle {
                                id: boardCell
                                required property int index
                                readonly property int cIdx: boardCell.index
                                readonly property int val: rootWindow.board[boardRow.rIdx * 4 + boardCell.cIdx]

                                width: 9
                                height: 3
                                color: rootWindow.getCellBgColor(boardCell.val)
                                border.style: rootWindow.getCellBorderStyle(boardCell.val)
                                border.color: rootWindow.getCellBorderColor(boardCell.val)

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: boardCell.val > 0 ? String(boardCell.val) : ""
                                    color: rootWindow.getCellTextColor(boardCell.val)
                                    bold: boardCell.val >= 8
                                    horizontalAlignment: Qt.AlignHCenter
                                    verticalAlignment: Qt.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
        }

        // Banner Overlays for Win / Loss States
        Rectangle {
            width: 44
            height: 3
            border.style: "single"
            border.color: rootWindow.won ? "magenta" : (rootWindow.gameOver ? "red" : "transparent")
            color: rootWindow.won ? "#2d002d" : (rootWindow.gameOver ? "#2d0000" : "transparent")
            visible: rootWindow.won || rootWindow.gameOver
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.centerIn: parent
                text: rootWindow.won ? "🎉 CONGRATULATIONS! YOU WON 2048! 🎉" : "💀 GAME OVER! Press [R] to Restart"
                color: rootWindow.won ? "magenta" : "red"
                bold: true
            }
        }
    }
}
