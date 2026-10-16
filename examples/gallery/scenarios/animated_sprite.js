/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("AnimatedSprite");
    terminal.expectText("Inspector");
    terminal.waitForRegex("loading: frame [0-9]+/4");
    terminal.expectText("of a 256x64 sheet");
    terminal.sleep(500);

    terminal.press("Space");
    terminal.waitForText("paused");
    terminal.sleep(400);

    terminal.press("Right");
    terminal.waitForRegex("loading tail: frame [0-9]+/2");
    terminal.sleep(400);

    terminal.typeText("r");
    terminal.waitForText("reverse");
    terminal.sleep(400);

    terminal.press("Space");
    terminal.expectText("[PLAYING]");
    terminal.sleep(500);
}
