/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForRegex("loading: frame [0-9]+/4");
    terminal.expectText("AnimatedSprite");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.waitForText("paused");
    terminal.sleep(700);
    terminal.press("Right");
    terminal.waitForRegex("loading tail: frame [0-9]+/2");
    terminal.sleep(700);
    terminal.typeText("r");
    terminal.waitForText("reverse");
    terminal.sleep(800);
    terminal.press("Space");
    terminal.sleep(900);
    terminal.captureText("animated_sprite.txt");
    terminal.stopRecording();
}
