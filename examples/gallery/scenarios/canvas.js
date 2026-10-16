/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Canvas");
    terminal.expectText("Inspector");
    terminal.expectText("Canvas.Braille");
    terminal.expectText("phase #0");
    terminal.sleep(500);

    terminal.press("Space");
    terminal.expectText("phase #1");
    terminal.sleep(400);

    terminal.press("Space");
    terminal.expectText("phase #2");
    terminal.sleep(400);

    terminal.press("m");
    terminal.expectText("Canvas.HalfBlock");
    terminal.sleep(500);

    terminal.clickText("Step Wave [Space]");
    terminal.expectText("phase #3");
    terminal.sleep(450);

    terminal.press("m");
    terminal.expectText("Canvas.Braille");
    terminal.sleep(500);
}
