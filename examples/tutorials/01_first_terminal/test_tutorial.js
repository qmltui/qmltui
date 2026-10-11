/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Hello, terminal!");
    terminal.captureText("tutorial.txt");
}
