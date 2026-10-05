// Frame cost must be observable from scenarios and recorded in result.json so
// rendering performance can be tracked alongside behaviour.
Test.waitForText("TESTRUNNER FIXTURE", 2000);
Test.sleep(250);            // let the fixture's one-shot timer commit its frame
const stats = Test.frameStats();
if (!(stats.frames > 0))
    Test.fail("frameStats().frames should be positive, got " + JSON.stringify(stats));
if (stats.frame !== Test.frameCount())
    Test.fail("frameStats().frame " + stats.frame + " does not match frameCount() " + Test.frameCount());
if (!(stats.paintMicros >= 0) || !(stats.diffMicros >= 0) || !(stats.paintMicrosMax >= stats.paintMicros))
    Test.fail("frame timings are inconsistent: " + JSON.stringify(stats));
if (!(stats.bytesTotal > 0))
    Test.fail("committed frames should have produced diff bytes: " + JSON.stringify(stats));
if (stats.dirtyDuringPaint !== 0)
    Test.fail("the fixture should not invalidate while painting: " + JSON.stringify(stats));
