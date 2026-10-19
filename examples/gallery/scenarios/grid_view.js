/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("apple");
    terminal.expectText("mango");
    terminal.sleep(800);
    terminal.press("Down");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.sleep(800);
    terminal.captureText("grid_view.txt");
    terminal.stopRecording();
}
