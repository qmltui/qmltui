// Frame cost must be observable from scenarios and recorded in result.json so
// rendering performance can be tracked alongside behaviour.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE", 2000);
    terminal.waitForText("STATUS: ASYNC READY", 2000);
    const stats = terminal.frameStats();
    if (!(stats.frames > 0))
        terminal.fail("frameStats().frames should be positive, got " + JSON.stringify(stats));
    if (stats.frame !== terminal.frameCount())
        terminal.fail("frameStats().frame " + stats.frame + " does not match frameCount() " + terminal.frameCount());
    if (!(stats.paintMicros >= 0) || !(stats.diffMicros >= 0) || !(stats.paintMicrosMax >= stats.paintMicros))
        terminal.fail("frame timings are inconsistent: " + JSON.stringify(stats));
    if (!(stats.bytesTotal > 0))
        terminal.fail("committed frames should have produced diff bytes: " + JSON.stringify(stats));
    if (stats.dirtyDuringPaint !== 0)
        terminal.fail("the fixture should not invalidate while painting: " + JSON.stringify(stats));
}
