/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("SERVICE MONITOR");
    terminal.expectText("auth-service");
    terminal.expectText("▲ degraded");

    // "q" is a letter in the focused filter, not a quit key.
    terminal.typeText("queue");
    terminal.waitForText("worker-queue");
    terminal.expectNoText("auth-service");

    // Escape clears the filter instead of quitting.
    terminal.press("Escape");
    terminal.waitForText("auth-service");

    // Telemetry driven from the script: no wall-clock wait.
    terminal.rootObject().tick();
    terminal.waitForText("11 ms"); // auth-service: 12 ms plus the first jitter step

    terminal.typeText("nothing");
    terminal.waitForText("no service matches");
    terminal.captureText("cpp-only.txt");
}
