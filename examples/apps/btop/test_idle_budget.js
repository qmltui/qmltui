// An idle application must not keep rendering. Test.sleep() runs the event
// loop without requesting a render, so every frame committed during the sleep
// was scheduled by the application itself. btop has a 1 s clock timer and a
// periodic model refresh, so at most two frames may legitimately land in a
// 500 ms window.
const BUDGET = 2;
Test.waitForText("PROCESSES", 2000);
Test.sleep(300);            // let startup settle
const before = Test.frameCount();
Test.sleep(500);
const advanced = Test.frameCount() - before;
if (advanced > BUDGET)
    Test.fail(`idle frame budget exceeded: ${advanced} frames in 500 ms (budget ${BUDGET}); `
              + JSON.stringify(Test.frameStats()));
