function main(app) {
    const sleepStartedAt = Date.now();
    app.sleep(25);
    if (Date.now() - sleepStartedAt < 20)
        app.fail("app.sleep() returned too early");

    let checks = 0;
    app.waitForCondition((frameText, elapsedMs) => {
        checks += 1;
        return checks >= 3
            && elapsedMs >= 0
            && frameText.includes("TESTRUNNER FIXTURE");
    }, 500);

    app.captureText("waits.txt");
    app.finish();
}
