/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Loading network requests (Dots)");
    terminal.advanceTime(200);
    terminal.captureText("busy_indicator.txt");
}
