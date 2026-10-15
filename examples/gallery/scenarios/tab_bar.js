/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("3 services healthy");
    terminal.sleep(800);
    terminal.press("Right");
    terminal.expectText("p99 latency 38 ms");
    terminal.sleep(800);
    terminal.press("Right");
    terminal.expectText("retrying upstream");
    terminal.sleep(900);
    terminal.captureText("tab_bar.txt");
    terminal.stopRecording();
}
