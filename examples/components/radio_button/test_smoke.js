/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui RadioButton Component Example");
    terminal.advanceTime(200);
    terminal.captureText("radio_button.txt");
}
