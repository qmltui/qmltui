/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Rectangle");
    terminal.expectText("Inspector");
    terminal.expectText("Border.Single");
    terminal.expectText("Gradient.Horizontal");
    terminal.sleep(500);

    terminal.press("b");
    terminal.expectText("Border.Double");
    terminal.sleep(450);

    terminal.press("b");
    terminal.expectText("Border.Rounded");
    terminal.sleep(450);

    terminal.press("b");
    terminal.expectText("Border.Ascii");
    terminal.sleep(450);

    terminal.press("g");
    terminal.expectText("Gradient.Vertical");
    terminal.sleep(500);

    terminal.clickText("Cycle Border [B]");
    terminal.expectText("Border.Single");
    terminal.sleep(500);
}
