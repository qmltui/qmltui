// Frame cost must be observable from scenarios and recorded in result.json so
// rendering performance can be tracked alongside behaviour.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE", 2000);
    app.waitForText("STATUS: ASYNC READY", 2000);
    const stats = app.frameStats();
    if (!(stats.frames > 0))
        app.fail("frameStats().frames should be positive, got " + JSON.stringify(stats));
    if (stats.frame !== app.frameCount())
        app.fail("frameStats().frame " + stats.frame + " does not match frameCount() " + app.frameCount());
    if (!(stats.paintMicros >= 0) || !(stats.diffMicros >= 0) || !(stats.paintMicrosMax >= stats.paintMicros))
        app.fail("frame timings are inconsistent: " + JSON.stringify(stats));
    if (!(stats.bytesTotal > 0))
        app.fail("committed frames should have produced diff bytes: " + JSON.stringify(stats));
    if (stats.dirtyDuringPaint !== 0)
        app.fail("the fixture should not invalidate while painting: " + JSON.stringify(stats));
}
