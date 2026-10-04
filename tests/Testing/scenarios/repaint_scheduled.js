// Changing application state must make the application schedule its own
// repaint. injectKey() sends the key without waiting for a frame, so
// expectRepaint() observes the application's own invalidation rather than one
// the harness requested. The fixture maps "k" to a status change.
Test.waitForText("TESTRUNNER FIXTURE", 2000);
Test.sleep(250);            // let the fixture's one-shot timer settle
const before = Test.frameCount();
Test.injectKey("k");
Test.expectRepaint(1000);
Test.expectText("STATUS: KEY K");
if (Test.frameCount() <= before) {
    Test.fail("frame sequence did not advance: " + before + " -> " + Test.frameCount());
}
