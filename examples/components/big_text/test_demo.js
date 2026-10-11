/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("BIGTEXT BANNER DEMO");
    terminal.sleep(3000);
    terminal.stopRecording();
}
