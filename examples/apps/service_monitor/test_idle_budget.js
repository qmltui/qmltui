// An idle application must not keep rendering. Test.sleep() runs the event
// loop without requesting a render, so every frame committed during the sleep
// was scheduled by the application itself. Service Monitor has no timers, so
// the budget is zero.
const BUDGET = 0;
Test.waitForText("SERVICE MONITOR", 2000);
Test.sleep(300);            // let startup settle
const before = Test.frameCount();
Test.sleep(500);
const advanced = Test.frameCount() - before;
if (advanced > BUDGET)
    Test.fail(`idle frame budget exceeded: ${advanced} frames in 500 ms (budget ${BUDGET}); `
              + JSON.stringify(Test.frameStats()));
