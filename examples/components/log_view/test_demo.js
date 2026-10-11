/** @param {Terminal} terminal */
function main(terminal) {
    // LogView loads initial rows via a QML timer; advance virtual time to fire it.
    terminal.advanceTime(500);
    terminal.expectText("gateway ready");
    terminal.sleep(2000);
    terminal.advanceTime(500);
    terminal.sleep(1500);
    terminal.stopRecording();
}
