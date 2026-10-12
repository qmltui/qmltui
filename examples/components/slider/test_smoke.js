/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Slider Component Example");
    terminal.advanceTime(200);
    terminal.captureText("slider.txt");
}
