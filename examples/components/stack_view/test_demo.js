/** @param {Terminal} terminal */
function main(terminal) {
    // Push button has focus on startup; press Enter directly to push.
    terminal.expectText("Page 1");
    terminal.sleep(1500);
    terminal.press("Enter");
    terminal.expectText("Page 2");
    terminal.sleep(1500);
    terminal.press("Enter");
    terminal.expectText("Page 3");
    terminal.sleep(1500);
    terminal.clickText("Pop");
    terminal.expectText("Page 2");
    terminal.sleep(1000);
    terminal.clickText("Pop");
    terminal.expectText("Page 1");
    terminal.sleep(1500);
    terminal.stopRecording();
}
