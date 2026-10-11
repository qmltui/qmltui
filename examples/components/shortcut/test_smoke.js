/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("theme: amber");
    terminal.press("2");
    terminal.expectText("theme: mint");
    terminal.press("F5");
    terminal.expectText("F5 pressed 1");
    terminal.press("Space");
    terminal.press("F5");
    terminal.expectText("F5 pressed 1");
    terminal.captureText("shortcut.txt");
}
