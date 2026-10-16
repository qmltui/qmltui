/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Cluster Subsystem Power Switches");
    terminal.expectText("online: 3 / 4");
    terminal.expectText("memory: 1152 MB");
    terminal.expectText("health: HEALTHY");

    // Toggle focused Primary Database switch off with Space
    terminal.press("SPACE");
    terminal.waitForText("online: 2 / 4");
    terminal.expectText("memory: 640 MB");
    terminal.expectText("health: DEGRADED");
    terminal.expectText("last: Database: OFF");

    // Click Telemetry Exporter switch to turn it on
    terminal.clickText("Telemetry Exporter");
    terminal.waitForText("online: 3 / 4");
    terminal.expectText("memory: 768 MB");

    // Click All On button in Inspector
    terminal.clickText("All On");
    terminal.waitForText("online: 4 / 4");
    terminal.expectText("memory: 1280 MB");
    terminal.expectText("health: OPTIMAL");

    // Click Kill button in Inspector
    terminal.clickText("Kill");
    terminal.waitForText("online: 0 / 4");
    terminal.expectText("memory: 0 MB");
}
