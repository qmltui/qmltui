/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Content Area 1");
    terminal.sleep(3000);
    terminal.stopRecording();
}
