/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Inline mode");
    terminal.expectScrollbackText("QmlTui");
    terminal.expectScrollbackText("Inline Agent CLI");
    terminal.sleep(700);
    terminal.typeText("deploy production --canary");
    terminal.sleep(600);
    terminal.press("Enter");
    terminal.expectText("printed: 1");
    terminal.expectScrollbackText("deploy production --canary");
    terminal.sleep(900);
    terminal.captureScrollback("inline_mode_scrollback.txt");
    terminal.captureText("inline_mode.txt");
    terminal.stopRecording();
}
