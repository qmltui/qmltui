function main(terminal) {
    terminal.waitForText("SNAKE");
    terminal.expectText("SCORE");
    terminal.captureText("snake/start.txt");

    // Maneuver the snake around the grid
    terminal.press("DOWN");
    terminal.sleep(150);
    terminal.press("LEFT");
    terminal.sleep(150);
    terminal.press("UP");
    terminal.sleep(150);
    terminal.press("RIGHT");
    terminal.sleep(150);
    terminal.captureText("snake/maneuvers.txt");

    // The tail must leave no trail. A block glyph in default ink means an
    // erased cell was painted "transparent" (default ink) instead of unset.
    const rows = terminal.frameText().split("\n");
    for (let y = 0; y < rows.length; ++y) {
        for (let x = 0; x < rows[y].length; ++x) {
            const glyph = rows[y][x];
            if (glyph !== "█" && glyph !== "▀" && glyph !== "▄")
                continue;
            if (terminal.cell(x, y).foreground === "#00000000")
                terminal.fail("default-ink block at " + x + "," + y + ": the snake left a trail");
        }
    }

    // Pause the game
    terminal.press("SPACE");
    terminal.waitForText("PAUSED");
    terminal.captureText("snake/paused.txt");

    // Resume and steer down-right
    terminal.press("SPACE");
    terminal.press("DOWN");
    terminal.sleep(150);
    terminal.press("RIGHT");
    terminal.sleep(150);

    // Restart the game
    terminal.press("R");
    terminal.waitForText("SCORE 0");
    terminal.captureText("snake/restarted.txt");

    terminal.finish();
}
