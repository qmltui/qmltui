/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Activity Indicators");
    terminal.sleep(800);
    terminal.advanceTime(200);
    terminal.sleep(600);
    terminal.advanceTime(200);
    terminal.sleep(600);
    terminal.advanceTime(200);
    terminal.sleep(600);
    terminal.advanceTime(200);
    terminal.sleep(600);
    terminal.advanceTime(200);
    terminal.sleep(1000);
    terminal.stopRecording();
}
