// The radar sweep served by an asynchronous provider changes as the tick
// advances, is drawn at the Image's renderSize, and pausing stops it.
// Under the test backend no pixel protocol exists, so it renders in half-block.
function main(app) {
    app.waitForText("Live Image: pictures from memory");
    app.waitForRegex("Tick [2-9]");
    app.expectText("source: image://frames/radar");
    const root = app.rootObject();
    const imageColumns = root.width - 6;
    const imageRows = root.height - 12;
    app.expectText("frames " + imageColumns + "x" + (imageRows * 2) + " px");
    app.expectText("\u2580");                     // half-block pixels on the page
    const early = app.frameText();
    app.captureText("live_image_early.txt");
    app.captureStyled("live_image_early_styled.txt");
    app.waitForRegex("Tick (1[0-9]|[6-9])");   // a few ticks later
    const later = app.frameText();
    if (later === early)
        app.fail("the pictures did not change between ticks");
    app.captureText("live_image_later.txt");
    app.captureStyled("live_image_later_styled.txt");
    app.press("Space");
    app.waitForText("(paused)");
    const paused = app.frameText();
    app.sleep(350);   // wall time: the asynchronous C++ provider must get a chance to tick
    app.waitForFrame();
    if (app.frameText() !== paused)
        app.fail("paused pictures must not change");
    app.captureText("live_image_paused.txt");
    app.captureStyled("live_image_paused_styled.txt");
}
