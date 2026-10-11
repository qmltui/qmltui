/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Popup & Z-Index Demo");
    terminal.sleep(1500);
    terminal.press("O");
    terminal.expectText("cannot be undone");
    terminal.sleep(2000);
    terminal.press("Escape");
    terminal.expectText("Qamui Popup & Z-Index Demo");
    terminal.sleep(1000);
    terminal.press("O");
    terminal.expectText("cannot be undone");
    terminal.sleep(1500);
    terminal.stopRecording();
}
