
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("QmlTui.Image");
    // No kitty graphics under the headless test backend, so Auto reports the
    // half-block fallback and cycling modes updates the status line.
    terminal.waitForText("half-block fallback");
    terminal.waitForText("Mode: Auto");
    terminal.captureText("image_demo/auto.txt");

    terminal.press("M");
    terminal.waitForText("Mode: HalfBlock");
    terminal.press("M");
    terminal.waitForText("Mode: Braille");
    terminal.press("M");
    terminal.waitForText("Mode: ASCII");

    terminal.press("2");
    terminal.waitForText("Image: demo_image.png");

    terminal.finish();
}
