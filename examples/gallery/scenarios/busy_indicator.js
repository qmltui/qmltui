/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Async Pipeline Spinner Monitor");
    terminal.expectText("state: Running");
    terminal.expectText("interval: 100ms");

    // Advance virtual clock so spinners animate
    terminal.sleep(120);

    // Press Space on focused running CheckBox to pause
    terminal.press("SPACE");
    terminal.waitForText("state: Paused");
    terminal.expectText("Pipeline: Paused");

    // Press RIGHT to increase interval by 20ms -> 120ms
    terminal.press("RIGHT");
    terminal.waitForText("interval: 120ms");

    // Click -20ms button twice -> 80ms
    terminal.clickText("-20ms");
    terminal.waitForText("interval: 100ms");
    terminal.clickText("-20ms");
    terminal.waitForText("interval: 80ms");

    // Click running checkbox to resume
    terminal.clickText("running");
    terminal.waitForText("state: Running");
}
