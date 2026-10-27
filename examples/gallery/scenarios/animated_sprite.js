/** @param {Runner} app */
function main(app) {
    app.waitForRegex("loading: frame [0-9]+/4");
    app.expectText("of 256x64 sheet");
    app.sleep(800);
    app.press("Space");
    app.waitForText("paused");
    app.sleep(700);
    app.press("Right");
    app.waitForRegex("loading tail: frame [0-9]+/2");
    app.sleep(700);
    app.typeText("r");
    app.waitForText("reverse");
    app.sleep(800);
    app.press("Space");
    app.sleep(900);
    app.captureText("animated_sprite.txt");
    app.stopRecording();
}
