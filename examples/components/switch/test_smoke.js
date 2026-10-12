/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Switch Component Example");
    terminal.advanceTime(200);
    terminal.captureText("switch.txt");
}
