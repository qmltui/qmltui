/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("appendLine()");
    terminal.sleep(500);
    terminal.sleep(500);
    terminal.sleep(500);
    terminal.expectText("gateway ready");
    terminal.sleep(500);
    terminal.captureText("log_view.txt");
    terminal.stopRecording();
}
