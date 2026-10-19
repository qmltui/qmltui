/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("cpu %");
    terminal.expectText("net MB/s");
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.sleep(400);
    terminal.captureText("sparkline.txt");
    terminal.stopRecording();
}
