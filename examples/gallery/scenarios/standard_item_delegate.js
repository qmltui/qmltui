/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Current: Build");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.expectText("Current: Run tests");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.expectText("Current: Tag release");
    terminal.sleep(900);
    terminal.captureText("standard_item_delegate.txt");
    terminal.stopRecording();
}
