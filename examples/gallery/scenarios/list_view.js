/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("ListView · Deployment Pipeline");
    terminal.expectText("fetch-sources");
    terminal.expectText("currentIndex: 0");
    terminal.expectText("State: DONE");
    terminal.expectText("spacing: 0");
    terminal.marker("Initial list state");
    terminal.sleep(400);

    // Navigate down to compile-core and activate it.
    terminal.press("Down");
    terminal.press("Down");
    terminal.waitForText("currentIndex: 2");
    terminal.expectText("Job:   compile-core");
    terminal.expectText("State: RUNNING");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.waitForText("Run: compile-core");
    terminal.marker("Job activated");
    terminal.sleep(400);

    // Toggle row spacing with S.
    terminal.typeText("s");
    terminal.waitForText("spacing: 1");
    terminal.expectText("spacing:1");
    terminal.sleep(300);

    // Move to lint-qml-strict (READY) and restore compact spacing.
    terminal.press("Down");
    terminal.press("Down");
    terminal.waitForText("currentIndex: 4");
    terminal.expectText("Job:   lint-qml-strict");
    terminal.expectText("State: READY");
    terminal.typeText("s");
    terminal.waitForText("spacing: 0");
    terminal.sleep(300);
}
