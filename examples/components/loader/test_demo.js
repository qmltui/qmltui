/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Loader Demo");
    terminal.sleep(1500);
    terminal.press("Space");
    terminal.sleep(1200);
    terminal.press("Space");
    terminal.sleep(1000);
    terminal.press("S");
    terminal.sleep(1200);
    terminal.press("S");
    terminal.sleep(1500);
    terminal.stopRecording();
}
