/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Image");
    terminal.expectText("Inspector");
    terminal.expectText("Image.HalfBlock");
    terminal.expectText("Image.Ready");
    terminal.sleep(500);

    terminal.press("m");
    terminal.expectText("Image.Braille");
    terminal.sleep(500);

    terminal.press("m");
    terminal.expectText("Image.ASCII");
    terminal.sleep(500);

    terminal.press("m");
    terminal.expectText("Image.Auto");
    terminal.sleep(500);

    terminal.press("s");
    terminal.expectText("smooth: true");
    terminal.sleep(450);

    terminal.press("m");
    terminal.expectText("Image.HalfBlock");
    terminal.sleep(450);
}
