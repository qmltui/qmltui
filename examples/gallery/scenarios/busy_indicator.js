/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Dots");
    terminal.expectText("Bounce");
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.captureText("busy_indicator.txt");
    terminal.stopRecording();
}
