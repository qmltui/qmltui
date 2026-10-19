/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Braille dot matrix");
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.captureText("canvas.txt");
    terminal.stopRecording();
}
