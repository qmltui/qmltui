/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("TableView · Process Monitor");
    terminal.expectText("Inspector");
    terminal.expectText("currentRow: 0");
    terminal.expectText("Name:  api-gateway");
    terminal.expectText("CPU%:  14.2%");
    terminal.expectText("State: RUNNING");
    terminal.marker("Initial table state");
    terminal.sleep(400);

    // Move down to the high-CPU auth-worker process and activate the row.
    terminal.press("Down");
    terminal.waitForText("currentRow: 1");
    terminal.expectText("Name:  auth-worker");
    terminal.expectText("CPU%:  84.6%");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.waitForText("Event: row#1 auth-worker");
    terminal.marker("Row activated");
    terminal.sleep(400);

    // Move down to the stopped cache-sync service.
    terminal.press("Down");
    terminal.press("Down");
    terminal.waitForText("currentRow: 3");
    terminal.expectText("Name:  cache-sync");
    terminal.expectText("State: STOPPED");
    terminal.sleep(300);

    // Toggle grid lines and alternating rows via keyboard shortcuts.
    terminal.typeText("g");
    terminal.waitForText("grid:off");
    terminal.sleep(300);

    terminal.typeText("g");
    terminal.waitForText("grid:on");
    terminal.typeText("a");
    terminal.sleep(300);
}
