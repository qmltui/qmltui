/** @param {Runner} app */
function main(app) {
    app.waitForText("SERVICE MONITOR");
    app.expectText("auth-service");
    app.expectText("▲ degraded");

    // "q" is a letter in the focused filter, not a quit key.
    app.typeText("queue");
    app.waitForText("worker-queue");
    app.expectNoText("auth-service");

    // Escape clears the filter instead of quitting.
    app.press("Escape");
    app.waitForText("auth-service");

    // Telemetry driven from the script: no wall-clock wait.
    app.rootObject().tick();
    app.waitForText("11 ms"); // auth-service: 12 ms plus the first jitter step

    app.typeText("nothing");
    app.waitForText("no service matches");
    app.captureText("cpp-only.txt");
}
