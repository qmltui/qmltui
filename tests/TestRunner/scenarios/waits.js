function main(terminal) {
    const sleepStartedAt = Date.now();
    terminal.sleep(25);
    if (Date.now() - sleepStartedAt < 20)
        terminal.fail("terminal.sleep() returned too early");

    let checks = 0;
    terminal.waitForCondition((frameText, elapsedMs) => {
        checks += 1;
        return checks >= 3
            && elapsedMs >= 0
            && frameText.includes("TESTRUNNER FIXTURE");
    }, 500);

    terminal.captureText("waits.txt");
    terminal.finish();
}
