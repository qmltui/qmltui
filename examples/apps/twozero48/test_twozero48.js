function main(terminal) {
    terminal.waitForText("2048 TUI");
    terminal.expectText("SCORE");
    terminal.expectText("BEST");
    terminal.captureText("twozero48/initial.txt");

    // Arrow moves
    terminal.press("UP");
    terminal.press("LEFT");
    terminal.press("DOWN");
    terminal.press("RIGHT");
    terminal.captureText("twozero48/arrow_moves.txt");

    // WASD moves
    terminal.typeText("w");
    terminal.typeText("a");
    terminal.typeText("s");
    terminal.typeText("d");
    terminal.captureText("twozero48/wasd_moves.txt");

    // Vim keys moves
    terminal.typeText("k");
    terminal.typeText("h");
    terminal.typeText("j");
    terminal.typeText("l");
    terminal.captureText("twozero48/vim_moves.txt");

    // Restart
    terminal.press("R");
    terminal.waitForText("SCORE");
    terminal.captureText("twozero48/restarted.txt");

    terminal.finish();
}
