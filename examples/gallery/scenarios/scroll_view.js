/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("contentY 0 of 40");
    terminal.sleep(700);
    terminal.press("PageDown");
    terminal.sleep(700);
    terminal.press("PageDown");
    terminal.sleep(800);
    terminal.captureText("scroll_view.txt");
    terminal.stopRecording();
}
