/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Interactive TextArea Component Demo");
    terminal.advanceTime(200);
    terminal.captureText("text_area.txt");
}
