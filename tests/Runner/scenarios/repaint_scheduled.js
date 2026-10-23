// Changing application state must make the application schedule its own
// repaint. injectKey() sends the key without waiting for a frame, so
// expectRepaint() observes the application's own invalidation rather than one
// the harness requested. The fixture maps "k" to a status change.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE", 2000);
    app.waitForText("STATUS: ASYNC READY", 2000);
    const before = app.frameCount();
    app.injectKey("k");
    app.expectRepaint(1000);
    app.expectText("STATUS: KEY K");
    if (app.frameCount() <= before) {
        app.fail("frame sequence did not advance: " + before + " -> " + app.frameCount());
    }
}
