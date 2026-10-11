/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Page 1");
    terminal.press("Enter");
    terminal.expectText("Page 2");
    terminal.clickText("Pop");
    terminal.expectText("Page 1");
    terminal.captureText("stack_view.txt");
}
