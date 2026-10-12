/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Dialog Example");
    terminal.advanceTime(200);
    terminal.captureText("dialog.txt");
}
