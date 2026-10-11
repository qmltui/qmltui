/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Gateway");
    terminal.press("Down");
    terminal.expectStyle(2, 4, { character: "W", bold: true });
    terminal.captureStyled("selection.txt");
    terminal.captureText("tutorial.txt");
}
