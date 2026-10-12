/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Popup & Z-Index Demo");
    terminal.advanceTime(200);
    terminal.captureText("popup.txt");
}
