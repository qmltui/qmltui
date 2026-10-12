/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui CheckBox Component Example");
    terminal.advanceTime(200);
    terminal.captureText("check_box.txt");
}
