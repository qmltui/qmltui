/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("No clicks yet");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.expectText("Clicked 1 time");
    terminal.sleep(700);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Enter");
    terminal.expectText("Clicked 2 times");
    terminal.sleep(700);
    terminal.clickText("Pin");
    terminal.expectText("Pinned");
    terminal.sleep(900);
    terminal.captureText("button.txt");
    terminal.stopRecording();
}
