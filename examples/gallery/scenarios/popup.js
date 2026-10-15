/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Opened 0 times");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Export options");
    terminal.expectText("Opened 1 time");
    terminal.sleep(900);
    terminal.clickText("Compress output");
    terminal.sleep(700);
    terminal.clickText("Export");
    terminal.sleep(900);
    terminal.captureText("popup.txt");
    terminal.stopRecording();
}
