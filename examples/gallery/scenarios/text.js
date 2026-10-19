/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("READY");
    terminal.expectText("gradient text flows");
    terminal.sleep(1200);
    terminal.captureText("text.txt");
    terminal.stopRecording();
}
