// Changing application state must make the application schedule its own
// repaint. injectKey() sends the key without waiting for a frame, so
// expectRepaint() observes the application's own invalidation rather than one
// the harness requested. The fixture maps "k" to a status change.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE", 2000);
    terminal.waitForText("STATUS: ASYNC READY", 2000);
    const before = terminal.frameCount();
    terminal.injectKey("k");
    terminal.expectRepaint(1000);
    terminal.expectText("STATUS: KEY K");
    if (terminal.frameCount() <= before) {
        terminal.fail("frame sequence did not advance: " + before + " -> " + terminal.frameCount());
    }
}
