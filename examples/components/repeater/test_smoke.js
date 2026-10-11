/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Count: 7");
    terminal.expectText("alpha");
    terminal.press("Right");
    terminal.expectText("Count: 8");
    terminal.captureText("repeater.txt");
}
