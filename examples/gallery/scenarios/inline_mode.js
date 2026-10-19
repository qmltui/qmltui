/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Inline mode");
    terminal.sleep(700);
    terminal.typeText("deploy production --canary");
    terminal.sleep(600);
    terminal.press("Enter");
    terminal.expectText("printed: 1");
    terminal.expectScrollbackText("deploy production --canary");
    terminal.sleep(900);
    terminal.captureText("inline_mode.txt");
    terminal.stopRecording();
}
