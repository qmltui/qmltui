// The radar sweep served by an asynchronous provider changes as the tick
// advances, is drawn at the Image's renderSize, and pausing stops it.
// Under the test backend no pixel protocol exists, so it renders in half-block.
function main(terminal) {
    terminal.waitForText("Live Image: pictures from memory");
    terminal.waitForRegex("Tick [2-9]");
    terminal.expectText("source: image://frames/radar");
    const root = terminal.rootObject();
    const imageColumns = root.width - 6;
    const imageRows = root.height - 12;
    terminal.expectText("frames " + imageColumns + "x" + (imageRows * 2) + " px");
    terminal.expectText("\u2580");                     // half-block pixels on the page
    const early = terminal.frameText();
    terminal.captureText("live_image_early.txt");
    terminal.captureStyled("live_image_early_styled.txt");
    terminal.waitForRegex("Tick (1[0-9]|[6-9])");   // a few ticks later
    const later = terminal.frameText();
    if (later === early)
        terminal.fail("the pictures did not change between ticks");
    terminal.captureText("live_image_later.txt");
    terminal.captureStyled("live_image_later_styled.txt");
    terminal.press("Space");
    terminal.waitForText("(paused)");
    const paused = terminal.frameText();
    terminal.sleep(350);
    terminal.waitForFrame();
    if (terminal.frameText() !== paused)
        terminal.fail("paused pictures must not change");
    terminal.captureText("live_image_paused.txt");
    terminal.captureStyled("live_image_paused_styled.txt");
}
