/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Loading sprite example");
    terminal.expectText("frame 1/4");
    terminal.expectText("[running]");

    // Advance 500ms to frame 2/4
    terminal.advanceTime(500);
    terminal.expectText("frame 2/4");

    // Advance 500ms to frame 3/4
    terminal.advanceTime(500);
    terminal.expectText("frame 3/4");

    // Test pause
    terminal.press("space");
    terminal.expectText("[paused]");
    terminal.advanceTime(1000);
    terminal.expectText("frame 3/4");

    // Test resume
    terminal.press("space");
    terminal.expectNoText("[paused]");

    // Advance to frame 4/4
    terminal.advanceTime(500);
    terminal.expectText("frame 4/4");

    // Advance to wrap around to frame 1/4
    terminal.advanceTime(500);
    terminal.expectText("frame 1/4");

    // Test reverse
    terminal.press("r");
    terminal.expectText("[reverse]");
    terminal.advanceTime(500);
    terminal.expectText("frame 4/4");

    // Snapshot test
    terminal.expectStyled("snapshots/loading_sprite.txt");

    terminal.press("q");
}
