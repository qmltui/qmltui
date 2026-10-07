function main(terminal) {
    terminal.waitForText("SNAKE");
    terminal.captureText("snake/ai_start.txt");

    let game = terminal.rootObject();

    function getNextMove(headX, headY, foodX, foodY, currentDir) {
        let dx = foodX - headX;
        let dy = foodY - headY;

        let preferred = [];
        if (Math.abs(dx) >= Math.abs(dy)) {
            if (dx > 0) preferred.push("RIGHT");
            else if (dx < 0) preferred.push("LEFT");
            if (dy > 0) preferred.push("DOWN");
            else if (dy < 0) preferred.push("UP");
        } else {
            if (dy > 0) preferred.push("DOWN");
            else if (dy < 0) preferred.push("UP");
            if (dx > 0) preferred.push("RIGHT");
            else if (dx < 0) preferred.push("LEFT");
        }

        let opposites = {
            "RIGHT": "LEFT",
            "LEFT": "RIGHT",
            "UP": "DOWN",
            "DOWN": "UP"
        };

        for (let i = 0; i < preferred.length; ++i) {
            let p = preferred[i];
            if (opposites[p] !== currentDir) {
                return p;
            }
        }
        return currentDir;
    }

    // Drive the snake to dynamically navigate towards food targets
    for (let step = 0; step < 40; ++step) {
        if (game.gameOver) {
            break;
        }

        let headX = game.headX;
        let headY = game.headY;
        let foodX = game.foodX;
        let foodY = game.foodY;
        let currentDir = game.direction.toUpperCase();

        let move = getNextMove(headX, headY, foodX, foodY, currentDir);
        if (move !== currentDir) {
            terminal.press(move);
        } else {
            terminal.sleep(60);
        }

        if (game.score >= 30) {
            break;
        }
    }

    terminal.captureText("snake/ai_gameplay.txt");
    terminal.finish();
}
