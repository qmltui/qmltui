/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("2 lines");
    terminal.sleep(700);
    terminal.press("End");
    terminal.press("Enter");
    terminal.typeText("Line 3 added live!");
    terminal.expectText("3 lines");
    terminal.sleep(900);
    terminal.captureText("text_area.txt");
    terminal.stopRecording();
}
