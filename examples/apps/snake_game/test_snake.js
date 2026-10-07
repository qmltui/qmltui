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
