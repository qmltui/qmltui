/** @param {Terminal} terminal */
function main(terminal) {
    terminal.advanceTime(400);
    terminal.expectText("gateway ready");
    terminal.advanceTime(400);
    terminal.expectText("request served in 12 ms");
    terminal.captureText("log_view.txt");
}
