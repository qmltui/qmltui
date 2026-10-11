/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Dialog Example");
    terminal.advanceTime(200);
    terminal.captureText("dialog.txt");
}
