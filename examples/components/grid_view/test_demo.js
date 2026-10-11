/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("apple");
    terminal.sleep(3000);
    terminal.stopRecording();
}
