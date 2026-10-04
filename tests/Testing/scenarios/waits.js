const sleepStartedAt = Date.now();
Test.sleep(25);
if (Date.now() - sleepStartedAt < 20)
    Test.fail("Test.sleep() returned too early");

let checks = 0;
Test.waitForCondition((frameText, elapsedMs) => {
    checks += 1;
    return checks >= 3
        && elapsedMs >= 0
        && frameText.includes("TESTRUNNER FIXTURE");
}, 500);

Test.captureText("waits.txt");
Test.finish();
