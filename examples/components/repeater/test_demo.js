/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Count: 7");
    terminal.sleep(3000);
    terminal.stopRecording();
}
