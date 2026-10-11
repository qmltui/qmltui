/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectSnapshot("snapshots/initial.txt");
    terminal.press("Space");
    terminal.expectText("Logging enabled");
    terminal.captureText("tutorial.txt");
}
