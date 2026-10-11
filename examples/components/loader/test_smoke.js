/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui Loader Demo");
    terminal.advanceTime(200);
    terminal.captureText("loader.txt");
}
