/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("single");
    terminal.expectText("double");
    terminal.expectText("rounded");
    terminal.expectText("horizontal gradient");
    terminal.sleep(1200);
    terminal.captureText("rectangle.txt");
    terminal.stopRecording();
}
