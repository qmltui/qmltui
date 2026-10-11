/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Switch Component Example");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.sleep(1500);
    terminal.stopRecording();
}
