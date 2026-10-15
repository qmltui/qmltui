/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Page 1");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Page 2");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Page 3");
    terminal.sleep(800);
    terminal.clickText("Pop");
    terminal.expectText("Page 2");
    terminal.sleep(900);
    terminal.captureText("stack_view.txt");
    terminal.stopRecording();
}
