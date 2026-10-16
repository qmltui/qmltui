/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Panel Chrome Stage");
    terminal.expectText("Inspector");
    terminal.expectText("Border Style: Rounded");
    terminal.expectText("Title Align:  Left");
    terminal.sleep(300);

    terminal.press("b");
    terminal.expectText("Border Style: Single");
    terminal.sleep(300);

    terminal.press("b");
    terminal.expectText("Border Style: Double");
    terminal.sleep(300);

    terminal.press("a");
    terminal.expectText("Title Align:  Center");
    terminal.sleep(300);

    terminal.press("g");
    terminal.expectText("no-badge");
    terminal.sleep(300);
}
