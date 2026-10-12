/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Button & ButtonGroup Component Example");
    terminal.advanceTime(200);
    terminal.captureText("button.txt");
}
