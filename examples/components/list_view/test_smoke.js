/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Employee Directory ---");
    terminal.advanceTime(200);
    terminal.captureText("list_view.txt");
}
