/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("MouseArea");
    terminal.expectText("Inspector");
    terminal.expectText("clicks: 0");
    terminal.sleep(450);

    terminal.hover(16, 8);
    terminal.expectText("containsMouse: true");
    terminal.sleep(450);

    terminal.click(18, 9);
    terminal.expectText("clicks: 1");
    terminal.expectText("LeftClick");
    terminal.sleep(450);

    terminal.click(24, 11, "right");
    terminal.expectText("clicks: 2");
    terminal.expectText("RightClick");
    terminal.sleep(450);

    terminal.scroll(20, 10, 0, 120);
    terminal.expectText("wheelSteps: 1");
    terminal.sleep(450);

    terminal.press("r");
    terminal.expectText("clicks: 0");
    terminal.expectText("Reset");
    terminal.sleep(450);
}
