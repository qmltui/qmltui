/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("segmented");
    terminal.expectText("thresholds");
    terminal.sleep(800);
    terminal.sleep(800);
    terminal.sleep(800);
    terminal.captureText("progress_bar.txt");
    terminal.stopRecording();
}
