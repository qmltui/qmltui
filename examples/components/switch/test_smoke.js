/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Switch Component Example");
    terminal.advanceTime(200);
    terminal.captureText("switch.txt");
}
