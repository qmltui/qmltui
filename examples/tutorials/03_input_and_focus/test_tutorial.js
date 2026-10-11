/** @param {Terminal} terminal */
function main(terminal) {
    terminal.typeText("Ada");
    terminal.press("Tab");
    terminal.press("Enter");
    terminal.expectText("Hello, Ada!");
    terminal.captureText("tutorial.txt");
}
