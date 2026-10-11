/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("BIGTEXT BANNER DEMO");
    terminal.advanceTime(200);
    terminal.captureText("big_text.txt");
}
