/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TABHEADER WIDGET DEMO");
    terminal.advanceTime(200);
    terminal.captureText("tab_bar.txt");
}
