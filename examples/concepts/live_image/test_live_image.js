// Both pictures are alive: the bound QImage and the provider-served one change
// as the tick advances, are drawn at the Image's renderSize, and pausing stops
// them. Under the test backend no pixel protocol exists, so both are half-block.
function main(terminal) {
    terminal.waitForText("Live Image: pictures from memory");
    terminal.waitForRegex("Tick [2-9]");
    terminal.expectText("image: feed.frame");
    terminal.expectText("source: image://frames/radar");
    terminal.expectText("frames 35x24 px");            // 35 cells x 12 rows, two pixels per cell
    terminal.expectText("\u2580");                     // half-block pixels on the page
    const early = terminal.frameText();
    terminal.captureText("live_image_early.txt");
    terminal.waitForRegex("Tick (1[0-9]|[6-9])");   // a few ticks later
    const later = terminal.frameText();
    if (later === early)
        terminal.fail("the pictures did not change between ticks");
    terminal.captureText("live_image_later.txt");
    terminal.press("Space");
    terminal.waitForText("(paused)");
    const paused = terminal.frameText();
    terminal.sleep(350);
    terminal.waitForFrame();
    if (terminal.frameText() !== paused)
        terminal.fail("paused pictures must not change");
    terminal.captureText("live_image_paused.txt");
}
